#ifndef FS_H_
#define FS_H_

void update_fs_stats(void);
struct fs_stat *prepare_fs_stat(const char *path);
void clear_fs_stats(void);

#endif /*FS_H_*/
