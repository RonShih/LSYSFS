/**
 * Less Simple, Yet Stupid Filesystem.
 * 
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing Less Simple, Yet Stupid Filesystem Using FUSE in C": http://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/
 *
 * License: GNU GPL
 */
 
#define FUSE_USE_VERSION 30
#define UNI_LIST_SIZE 30


#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// ... //

char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

static int count_ga = 0, count_ls = 0, count_rf = 0, count_md = 0, count_mf = 0, count_wf = 0,  count_rmf = 0, count_rmd = 0, count_ut = 0;//for observations

unsigned int dir_time[256][3] = {{0}};//[][0]: access time, [][1]: modified time, [][2]: change time
unsigned int files_time[256][3] = {{0}};

void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
}

int is_dir( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

int get_dir_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return curr_idx;
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );

	if ( file_idx == -1 ) // No such file
		return;
	
	if(strlen(files_content[file_idx]) == 0)//if the string is empty
		strcpy( files_content[ file_idx ], new_content ); 
	else
		strcat(files_content[file_idx], new_content ); 
}

// ... //

static int do_getattr( const char *path, struct stat *st )
{
	printf("getattr = %s, %d\n", path, ++count_ga);
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	
	int cur_index = 0;
	
	if ( strcmp( path, "/" ) == 0)
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		st->st_atime = time(NULL);
		st->st_mtime = time(NULL); 
		st->st_ctime = time(NULL);
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
		cur_index = get_file_index(path);
		if(files_time[cur_index][0] == 0 && files_time[cur_index][1] == 0 && files_time[cur_index][2] == 0)
		{
			st->st_atime = time(NULL);
			st->st_mtime = time(NULL);
			st->st_ctime = time(NULL); 
			files_time[cur_index][0] = st->st_atime;//maintain the old time
			files_time[cur_index][1] = st->st_mtime;
			files_time[cur_index][2] = st->st_ctime;
		}
		else
		{
			st->st_atime = files_time[cur_index][0];//get the old time
			st->st_mtime = files_time[cur_index][1];
			st->st_ctime = files_time[cur_index][2];
		}
	}
	else if( is_dir( path ) == 1) 
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		cur_index = get_dir_index(path);
		if(dir_time[cur_index][0] == 0 && dir_time[cur_index][1] == 0 && dir_time[cur_index][2] == 0)
		{
			st->st_atime = time(NULL);
			st->st_mtime = time(NULL); 
			dir_time[cur_index][0] = st->st_atime;
			dir_time[cur_index][1] = st->st_mtime;
			dir_time[cur_index][2] = st->st_ctime;
		}
		else
		{
			st->st_atime = dir_time[cur_index][0];
			st->st_mtime = dir_time[cur_index][1];
			st->st_ctime = dir_time[cur_index][2];
		}
	}
	else
	{
		printf("-ENOENT\n");
		return -ENOENT;
	}
	
	return 0;
}

int get_delim_count(const char *str)//number of '/'
{
	int count = 0;
	for(int i = 0 ; i < strlen(str); i++)
		if(str[i] == '/')
			count++;
	return count;
}

int get_first_delim(const char *str)//number of '/'
{
	int loc = 0;
	for(int i = 0 ; i < strlen(str); i++)
	{
		if(str[i] == '/')
		{
			loc = i;
			break;
		}
	}
	return loc;
}

int get_first_node(const char *dir, char *first_node, const int mode)//children path (dir) might not be a single node
{
	int first_delim_idx = -1, sec_delim_idx = -1, delim_count = 0, succeed = 0; ////find the first and second delim location
	for(int j = 0; j < strlen(dir); j++)
	{
		if(dir[j] == '/' && delim_count == 0)//get the first delim
		{
			first_delim_idx = j;
			delim_count++;
			continue;
		}
		else if(dir[j] =='/' && delim_count == 1)//get the last delim
		{	
			sec_delim_idx = j;
			delim_count++;
			break;
		}
	}
	
	if(mode == 1)//current path == root
	{
		if(delim_count>=1)//e.g. /a/b/c, /aaaa/b/c under root
			strncpy(first_node, dir, first_delim_idx);
		else
			strcpy(first_node, dir);//the dir is the first_node	
		succeed = 1;
	}
	else if(mode == 2)//current path != root
	{
		if(first_delim_idx == 0)//printf("not in root, first node = %s\n", first_node);
		{
			if(delim_count == 1)//only a node so skip the first '/'
				strncpy(first_node, &dir[1], strlen(dir)-1);
			else if(delim_count > 1)//skip the first and last '/'
				strncpy(first_node, &dir[1], sec_delim_idx - 1);
			return 1;		
		}
		else return 0;//e.g. under /aa/a subpath there's a subdir /aa/aa/b, it will find a/b in this case which is wrong
	}
	else return 0;
}

//avoid printing repeat data;
int no_repeat(const char *first_node,  const char uni_node_list[UNI_LIST_SIZE][256], const int uni_idx)
{
	int do_fill = 1;
	for(int i = 0; i < uni_idx; i++)
		if(strcmp(first_node, uni_node_list[i])==0)
			do_fill = 0;//repeat found
	return do_fill;
}

//check if the prefix of subpath is identical to dir
int is_prefix(const char *subpath, const char *dir)
{
	printf("%s, %s\n", subpath, dir);
	int count_pd = 0, count_dd = 0;
	char cur_dir_prefix[256] = "";
	if(get_delim_count(subpath) <= get_delim_count(dir) && strcmp(subpath, dir)<=0)//get prefix of dir_list split pathsize str from dir_list
	{
		strncpy(cur_dir_prefix, dir, strlen(subpath));
		if(strcmp(subpath, cur_dir_prefix) == 0)//comparison succeeds
			return 1;
	}
	return 0;
}


static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	printf("readdir = %d\n", ++count_ls);
	
	char subpath[256] = "";
	strncpy(subpath, &path[1], strlen(path)-1);//substr of path without '/' called subpath //printf("subpath is = %s\n", subpath);

	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 )//in root
	{
		int mode = 1, uni_node_idx = 0;//mode.1: in root
		char uni_node_list[UNI_LIST_SIZE][256];//to avoid repeat list
		for ( int i = 0; i <= curr_dir_idx; i++ )//traver dir_list
		{
			char first_node[256] = "";//first node under this dir
			printf("current dir = %s\n", dir_list[i]);
			if(get_first_node(dir_list[i], first_node, mode))//all dir are the chlidren of root, find the first node of dir 
			{	
				if(no_repeat(first_node, uni_node_list, uni_node_idx))//no reapet 
				{	
					strcpy(uni_node_list[uni_node_idx], first_node);
					uni_node_idx++;	
					filler( buffer, first_node, NULL, 0 );	
				}
			}
		}
		
		for ( int i = 0; i <= curr_file_idx; i++ )//traver file_list
		{
			char first_node[256] = "";//first node under this file
			printf("current file = %s\n", files_list[i]);
			if(get_first_node(files_list[i], first_node, mode))//all file are the chlidren of root, find the first node of file 
			{	
				if(no_repeat(first_node, uni_node_list, uni_node_idx))//no reapet 
				{	
					strcpy(uni_node_list[uni_node_idx], first_node);
					uni_node_idx++;	
					filler( buffer, first_node, NULL, 0 );	
				}
			}
		}
		
	}
	else//not in root
	{
		int mode = 2, uni_node_idx = 0;//mode.1: in root
		char uni_node_list[UNI_LIST_SIZE][256];//to avoid repeat list
		for(int i = 0 ; i <= curr_dir_idx; i++)//traver dir_list
		{
			if(is_prefix(subpath, dir_list[i])) //printf("current dir = %s\n", dir_list[i]);
			{
				printf("current dir = %s\n", dir_list[i]);
				int size_dpdif = strlen(dir_list[i])-strlen(subpath);//size difference between dir and path
				char first_node[256] = "", under_this_path[256] = "";//(1)first_node: node under this dir (2)under_this_path: must be this path's children
				strncpy(under_this_path, &dir_list[i][strlen(subpath)], size_dpdif);//copy last part of dir
				
				if(get_first_node(under_this_path, first_node, mode))//get ls node and no duplicate nodes	 
				{
					if(no_repeat(first_node, uni_node_list, uni_node_idx))//no repeat
					{		
						strcpy(uni_node_list[uni_node_idx], first_node);
						uni_node_idx++;
						filler(buffer, first_node, NULL, 0);//filler this node into buffer
					}	
				}
			}		
		}
		for(int i = 0 ; i <= curr_file_idx; i++)//traver file_list
		{
			if(is_prefix(subpath, files_list[i]))
			{
				printf("current file = %s\n", files_list[i]);
				int size_dpdif = strlen(files_list[i])-strlen(subpath);//size difference between file and path
				char first_node[256] = "", under_this_path[256] = "";//(1)first_node: node under this file (2)under_this_path: must be this path's children
				strncpy(under_this_path, &files_list[i][strlen(subpath)], size_dpdif);//copy last part of file
				
				if(get_first_node(under_this_path, first_node, mode))//get ls node and no duplicate nodes	 
				{
					if(no_repeat(first_node, uni_node_list, uni_node_idx))//no repeat
					{		
						strcpy(uni_node_list[uni_node_idx], first_node);
						uni_node_idx++;
						filler(buffer, first_node, NULL, 0);//filler this node into buffer
					}	
				}
			}		
		}
	}	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	printf("read file = %d\n", ++count_rf);
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}

static int do_mkdir( const char *path, mode_t mode )
{
	printf("mkdir = %d\n", ++count_md);
	path++;
	add_dir( path );
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	printf("mknod = %d\n", ++count_mf);
	path++;
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	printf("write = %d\n", ++count_wf);
	write_to_file( path, buffer );
	
	return size;
}

int do_unlink (const char *path)/** Remove a file */
{
	int file_idx = get_file_index(path), count = 0, found = -1, delete;
	char subpath[256] = "";
	
	if(file_idx == -1) return -1;//file not found
	
	strncpy(subpath, &path[1], strlen(path)-1);//substr of path without '/' called subpath //printf("subpath is = %s\n", subpath);
	printf("delete file = %d\n", ++count_rmf);

	for(int i = 0 ; i <= curr_file_idx; i++)
		if(strcmp(subpath, files_list[i]) == 0)
			found = 0;

	if(found == 0)
	{
		for(int i = file_idx; i < curr_file_idx && curr_file_idx == curr_file_content_idx; i++)//copy over the deleted index
		{
			strcpy(files_list[i], files_list[i+1]);//file list
			strcpy(files_content[i], files_content[i+1]);//file content list
			files_time[i][0] = files_time[i+1][0];
			files_time[i][1] = files_time[i+1][1];
		}
		curr_file_idx--;//shrink currnet size
		curr_file_content_idx--;//shrink current size
		return 0;
		
	}
	else return -1; //0: succeed, -1: false
}

	
int do_rmdir (const char *path)/** Remove a directory */
{
	int dir_idx = get_dir_index(path), count = 0, found = -1;
	char subpath[256] = "";
	
	if(dir_idx == -1) return -1;//dir not found
	
	strncpy(subpath, &path[1], strlen(path)-1);//substr of path without '/' called subpath //printf("subpath is = %s\n", subpath);

	printf("delete dir = %s, %d\n", subpath, ++count_rmd);
	
	for(int i = 0 ; i <= curr_dir_idx; i++)//case: dirs under a dir
	{
		if(is_prefix(subpath, dir_list[i]))//not removable if there're dirs under a dir
			count++;
		if(strcmp(subpath, dir_list[i]) == 0)//removable if cur_dir == delete target
			found = 0;
	}

	for(int i = 0 ; i <= curr_file_idx; i++)//not removable if files under a dir 
		if(is_prefix(subpath, files_list[i]))
			found = -1;

	if(count == 1 && found == 0)
	{
		strcpy(dir_list[dir_idx], "");//clear this string
		for(int i = dir_idx; i < curr_dir_idx; i++)//copy forward
		{
			strcpy(dir_list[i], dir_list[i+1]);
			dir_time[i][0] = dir_time[i+1][0];
			dir_time[i][1] = dir_time[i+1][1];
			dir_time[i][2] = dir_time[i+1][2];
		}
		curr_dir_idx--;//shrink
		return 0;
	}
	else return -1; //0: succeed, -1: false
}


int do_utime(const char *path, struct utimbuf *buf)
{
	printf("do_utime = %d\n", ++count_ut);
	int cur_index;
	buf->actime = time(NULL);
	buf->modtime = time(NULL);
	if(is_dir(path))
	{
		cur_index = get_dir_index(path);
		dir_time[cur_index][0] = buf->actime;
		dir_time[cur_index][1] = buf->modtime;
		return 0;
	}
	else if (is_file(path))
	{
		cur_index = get_file_index(path);
		files_time[cur_index][0] = buf->actime;
		files_time[cur_index][1] = buf->modtime;
		return 0;
	}
	else return -1;
}	

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
    .unlink		= do_unlink,
    .rmdir		= do_rmdir,
    .utime		= do_utime,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}