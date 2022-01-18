Less Simple, Yet Stupid Filesystem (Using FUSE)
=======================================

This is an example of using FUSE to build a simple in-memory filesystem that supports creating new files and directories. It is a part of a tutorial in MQH Blog with the title "Writing Less Simple, Yet Stupid Filesystem Using FUSE in C": <http://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/>

License: GNU GPL.

### 執行LSYSFS
* 檢查FUSE版本: fusermount version: [2.9.4](https://github.com/libfuse/libfuse/releases?page=5)
* 建立一個資料夾作為mount point
* make
* 執行lsysfs，最後一個參數為作為mount point的資料夾
```
~/Desktop/LSYSFS$ fusermount -V
~/Desktop/LSYSFS$ make
~/Desktop/LSYSFS$ mkdir ~/Desktop/mp
~/Desktop/LSYSFS$ ./lsysfs -f ~/Desktop/mp
```
* mount point出現問題時，將原資料夾umount或刪除
```
~/Desktop/LSYSFS$ sudo umount ~/Desktop/mp
~/Desktop/LSYSFS$ rm -d -r ~/Desktop/mp
```
### 調用fuse內部的functions
所有可調用的function可參考至[libfuse.h](https://github.com/libfuse/libfuse/blob/master/include/fuse.h)
```c=
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
```
* 資料結構的維護如下:
    1. dir_list, files_list所儲存的皆為一個path(e.g., dir1/dir1/dir1)
    2. files_content儲存對應files_list的資料
    3. dir_time, files_time所儲存的為access/modified/change time
    4. curr_dir_idx, curr_file_idx, curr_file_content_idx紀錄目前最後一筆資料的index
```c=
char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

unsigned int dir_time[256][3] = {{0}};//[][0]: access time, [][1]: modified time, [][2]: change time
unsigned int files_time[256][3] = {{0}};
```
* 對於lsysfs.c新增/更改functions如下:
    1. `do_getattr`:
        * 進行任何操作前都會進入此function
        * 獲取/賦予目前path狀態[stat](https://man7.org/linux/man-pages/man2/lstat.2.html)
        * <font color="blue">新增部分</font>:
            1. 每次getattr時不會更新access/modified time 
    2. `do_readdir`
        * 進行`ls`(list)操作時會進入此function
        * 使用`filler()`將目前path下的file與dir傳入buffer參數來印出
        * <font color="blue">新增部分</font>:
            1. 透過字串分割來達到進入多層的功能
            2. 在root內ls時只需印出dir或是root內的檔案，並只能印一個
            3. 非root內ls時需檢查dir或file是否為此path之下的物件，並只能印此dir下的dir以及file，且只能印一個
    4. `do_read`
        * 進行`cat`操作時會進入此function
        * 將內容傳入至buffer參數內印出
        * <font color="blue">新增部分</font>:
            1. 更新此file的access time
    6. `do_mkdir`(未更改)
        * 進行`mkdir`操作時會進入此function
    8. `do_mknod`(未更改)
        * 進行建檔相關操作時會進入此function
        * e.g., 用`touch`, `echo`建立新檔案
    10. `do_write`
        * 進行寫檔相關操作時會進入此function (e.g., `echo "string" >> file1`)
        * <font color="blue">新增部分</font>: 
            1. 日後新增至此file的字串皆須銜接至原字串之下方
    12. `do_unlink`
        * 刪除file時會進入此function (e.g., `rm file1`)
        * <font color="blue">新增部分</font>: 
            1. path為目前所處的dir+要刪除的目標
            2. 在files_list尋找刪除目標，並更新files_list, files_time相關資訊
            3. 回傳0代表成功，-1則失敗
    14. `do_rmdir`
        * 刪除dir時會進入此function (e.g., `rmdir dir1`)
        * <font color="blue">新增部分</font>: 
            1. path為目前所處的dir+要刪除的目標
            2. 須注意要刪除的目錄是否還包含dir或file
            3. 在dir_list尋找刪除目標，並更新dir_list, dir_time相關資訊
            4. 回傳0代表成功，-1則失敗
    16. `do_utime`
        * 更新時間的操作會進入此function (e.g., `touch file1`)
        * <font color="blue">新增部分</font>: 
            1. 更新`utimbuf`型態的[參數](https://man7.org/linux/man-pages/man2/utime.2.html)，更新其actime以及modtime至目前時間
            2. 更新對應dir_time或files_time
            3. 回傳0代表成功，-1則失敗

### References
* FUSE version-2.9.4
https://github.com/libfuse/libfuse/releases?page=5
* LSYSFS教學文
https://www.maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/
* LSYSFS github
https://github.com/MaaSTaaR/LSYSFS
* LSYSFS modified version github
https://github.com/RonShih/LSYSFS
* libfuse.h
https://github.com/libfuse/libfuse/blob/master/include/fuse.h
* stat struct
https://man7.org/linux/man-pages/man2/lstat.2.html
* utimbuf struct
https://man7.org/linux/man-pages/man2/utime.2.html
