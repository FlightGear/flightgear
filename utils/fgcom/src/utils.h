
#ifndef __UTILS_H__
#define __UTILS_H__

/* Initialize the file parser. */
int parser_init(const char *filename);

/* Exits parser. */
void parser_exit(void);

int parser_get_next_value(double *value);

extern int is_file_or_directory( const char * path ); /* 1=file, 2=dir, else 0 */
extern void trim_base_path_ib( char *path ); /* trim to base path IN BUFFER */
extern int get_data_path_per_os( char *path, size_t len ); /* get data path per OS - 0=ok, 1=error */

#endif
