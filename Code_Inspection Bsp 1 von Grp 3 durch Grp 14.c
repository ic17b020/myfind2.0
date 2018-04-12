/**
 * @file myfind.c
 *
 * SS-2018 - BES
 *
 * Beispiel 1 - myfind
 *
 * @author Roland Varga <ic17b073@technikum-wien.at>
 * @author Leonhard Meisel <ic17b033@technikum-wien.at> 
 * @author Fritz Schülein <ic16b081@technikum-wien.at>
 * @author Mahboobeh sadat Shafiee <ic17b074@technikum-wien.at>
 * @date 2018_03_12
 *
 * @version 1.0 
 */

/* -- Includes -- */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <fnmatch.h>

/* -- Defines -- */
#define CURRENT_DIR "."
#define PARENT_DIR  ".."

#define UID_GID_BUFFER_SIZE   11   /* 2^32+1 - 4 Byte + '\0' */
#define FILE_SIZE_BUFFER_SIZE  9   /* Max file length buffer size + '\0' */                                                                           

/* -- Prototypes -- */
static void do_file(const char * file_name, const char * const * parms);
static void do_dir(const char * dir_name, const char * const * parms);

static int path_combine(const char *src_path1, const char *src_path2, char *dest);
static const char *basename(const char *filename);
static void report_error(const char *msg, const char *arg);
static void print_info(void);
static void force_write_user_space(void);

static int param_print(const char *file_name);
static int param_ls(const char *file_name, const struct stat *file_info);

static int string_to_id(const char *id_str);
static int check_if_usr(const struct stat *file_info, const char *usr);
static int check_if_grp(const struct stat *file_info, const char *grp);
static int check_if_no_usr(const struct stat *file_info);
static int check_if_no_grp(const struct stat *file_info);
static int check_if_name(const char *file, const char *pattern);
static int check_if_path(const char *path, const char *pattern);
static int check_if_type(const struct stat *file_info, const char *flag);

/* -- Global vars -- */
static const char *PROG_NAME = "myfind_prog_name";

/**
 *\name main
 *
 *\brief Main function
 *
 *\param argc An integer that contains the number of parameters.
 *\param argv A const char pointer to command arguments.
 *
 *\return Returns EXIT_SUCCESS if successfully; otherwise EXIT_FAILURE. 
 *\retval EXIT_SUCCESS
 *\retval EXIT_FAILURE
 */
int main(int argc, const char *argv[]) {
    
    /*
     * ### FB: GRP14:   Hier wird überprüft, ob argv[0] leer bzw. NULL ist. Allerdings enthält argv[0] immer den Namen mit dem das
     *                  Programm aufgerufen wurde. Daher die Frage, warum diese Überprüfung überhaupt gemacht wird bzw. wie argv[0]
     *                  leer bzw. NULL sein könnte?
     */
  if(!argv[0]){         /* Error handling - Invalid parameter */                     
    
    report_error("An error occurred. Invalid parameter in function main.", NULL);
    return EXIT_FAILURE;
  }
  
  PROG_NAME = argv[0];  /* Set global var program name. Used in function 'report_error'error'. */  
   
  if (argc == 1) {     /* Error handling - No parameter */
  
      print_info();    /* print info */
      force_write_user_space();
  }
  
  do_file(argv[1], argv);
  
  if (fflush(stdout) == EOF) {  /* Error handling - Error handling - on error -> EOF and errno is set
                                                                     errno: EBADF - Stream is not an open stream, or is not open for writing. */
    
    report_error("An error occurred while force_write_user_space().", NULL);
  }	
  
  return EXIT_SUCCESS;	
}

/**
 *\name do_file
 *
 *\brief Gets the file properties and resolve the program parameters.
 *
 * Gets the file properties and evaluates the program parameters. Recursive usage with do_dir()
 *
 *\param file_name A const char pointer that contains the file name or path.
 *\param parms A const char pointer that contains the argument parameters.
 *
 *\return void Returns no value.
 */
static void do_file(const char *file_name, const char * const *parms) {
  
  struct stat file_info;
  
  int output_flag = 1; /* 0.. already printed; 1.. no print */
  int filter_flag = 0; /* 0.. no filter;       1.. filter   */ 
  
  errno = 0; /* Init. errno */ 
  if (lstat(file_name, &file_info) < 0) { /*  Error handling - on error -> -1 
                                                    errno:  EACCES    - Search  permission is denied for one of the directories in the path prefix of path.
                                                            EBADF     - fd is bad.
                                                            EFAULT    - Bad address.
                                                            ELOOP     - Too many symbolic links encountered while traversing the path.
                                                            ENAMETOOLONG - File name too long.
                                                            ENOENT    - A component of path does not exist, or path is an empty  string.
                                                            ENOMEM    - Out of memory (i.e., kernel memory).
                                                            ENOTDIR   - A component of the path prefix of path is not a directory.
                                                            EOVERFLOW - path refers to a file whose size cannot be represented in the type
                                            */
    
    report_error("An error occurred while lstat.", NULL);
    /* 
     * ### FB: GRP14: do_file ist void function und sollte keinen return value haben, 
     *                  return EXIT_FAILURE hat value 1 -> besser exit(EXIT_FAILURE) 
     *                  wie bspw. force_write_user_space() da exit() void function ist 
     */    
    return EXIT_FAILURE;
  }
      
  int cmd_count = 2;                                 /* start with the second command */
 
  while(parms[cmd_count] != NULL) {  /* solange bis ausführen bis keine Command-Parameter mehr da sind */
  
    /* Check parameter if '-name' */
    if(!strcmp(parms[cmd_count],"-name")) {
      
      if (!parms[cmd_count + 1]) {                    /* check if second parameter is missing */
        report_error("missing argument to ", parms[cmd_count]);
        force_write_user_space();
      }
      
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = check_if_name(file_name, parms[++cmd_count]);    
      } else {
        
        ++cmd_count;
      }
    }
    
    /* Check parameter if '-type' */
    else if (!strcmp(parms[cmd_count],"-type")){
      
      if (!parms[cmd_count + 1]) {                    /* Error handling. Second parameter is missing */
      
        report_error("missing argument to ",parms[cmd_count]);
        force_write_user_space();
      }
      
      if (strlen(parms[cmd_count + 1]) != 1) {        /* Error handling. Second parameter contains too many options */
        
        report_error("Arguments to -type should contain only one letter", NULL);
        force_write_user_space();
      }
      
      if (*parms[cmd_count + 1] != 'b' && 
          *parms[cmd_count + 1] != 'c' &&
					*parms[cmd_count + 1] != 'd' && 
          *parms[cmd_count + 1] != 'p' && 
				  *parms[cmd_count + 1] != 'l' && 
          *parms[cmd_count + 1] != 'f' &&
				  *parms[cmd_count + 1] != 's') {           /* Error handling. Second parameter contains unknown argument */
            
        report_error("Unknown argument to -type", NULL);
        force_write_user_space();
      }
      
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = check_if_type(&file_info, parms[++cmd_count]);      
      } else {
        
        ++cmd_count;
      }
    }
    
    /* Check parameter if '-print' */
    else if(!strcmp(parms[cmd_count],"-print")){
      
      output_flag = param_print(!filter_flag ? file_name : NULL);
    }
    
    /* Check parameter if '-ls' */
    else if(!strcmp(parms[cmd_count],"-ls")){
      
      output_flag = param_ls(!filter_flag ? file_name : NULL, &file_info);
    }
   
    /* Check parameter if '-path' */
    else if(!strcmp(parms[cmd_count],"-path")) {
      
      if (!parms[cmd_count + 1]) {                    /* check if second parameter is missing */
        report_error("missing argument to ", parms[cmd_count]);
        force_write_user_space();
      }
            
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = check_if_path(file_name, parms[++cmd_count]);    
      } else {
        
        ++cmd_count;
      }
    }

    /*Check parameter if '-user'*/ 
    else if(!strcmp(parms[cmd_count],"-user")) {
        
      if (!parms[cmd_count + 1]) {                    /* check if second parameter is missing */
        report_error("missing argument to ", parms[cmd_count]);
        force_write_user_space();
      }
      
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = check_if_usr(&file_info, parms[++cmd_count]);    
      } else {
        
        ++cmd_count;
      }
    }

   
    /* Check parameter if '-group' */
    else if(!strcmp(parms[cmd_count],"-group")){
      if (!parms[cmd_count + 1]) {                    /* check if second parameter is missing */
        report_error("missing argument to ", parms[cmd_count]);
        force_write_user_space();
      }

      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = check_if_grp(&file_info, parms[++cmd_count]);    
      } else {
        
        ++cmd_count;
      }
    }
 
    /* Check parameter if '-nouser' */
    else if(!strcmp(parms[cmd_count],"-nouser")){
      
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = !check_if_no_usr(&file_info);    
      } 
    }
    
    /* Check parameter if '-nogroup' */
    else if(!strcmp(parms[cmd_count],"-nogroup")){
    
      if (!filter_flag) { /* of filter already enabled do not check file name again*/
      
        filter_flag = !check_if_no_grp(&file_info);    
      } 
    }
      
    else {
      report_error("unknown predicate ", parms[cmd_count]);
      force_write_user_space();
    }
    
    cmd_count++; 
  }
  
  /* if no print or ls parameter is set (output_flag != 0) */
  if(output_flag) {
    
   param_print(!filter_flag ? file_name : NULL );
  }
  
  /* Recursive call to do_dir() */
  if(S_ISDIR(file_info.st_mode)) {

    do_dir(file_name, parms);
  }
   
  return;
}

/**
 *\name do_dir
 *
 *\brief This functions opens a directory and iterates through. 
 *
 * Works recursive with do_file(). 
 *
 *\param dir_name A char pointer that contains the directory name.
 *\param parms A const char pointer that contains the argument parameters.
 *
 *\return Returns no value.
 */
static void do_dir(const char *dir_name, const char * const * parms) {
  
  DIR *path                = NULL;
  struct dirent *dir_entry = NULL;
  
  errno = 0; /* Init. errno */
  if (!(path = opendir(dir_name)) && errno != 0) {  /* Error handling - on error -> NULL 
                                                                errno:  EACCES  - Permission denied. 
                                                                        EBADF   - fd is not a valid file descriptor opened for reading. 
                                                                        EMFILE  - Too many file descriptors in use by process. 
                                                                        ENFILE  - Too many files are currently open in the system. 
                                                                        ENOENT  - Directory does not exist, or name is an empty string. 
                                                                        ENOMEM  - Insufficient memory to complete the operation. 
                                                                        ENOTDIR - name is not a directory.  */
    
    report_error("An error occurred while opendir.", NULL);                             
    force_write_user_space();
    
  };
  
  errno = 0; /* Init. errno */
  while((dir_entry = readdir(path))) {  
  
    if (dir_entry != NULL && errno != 0) { /* Error handling - on error -> -1
                                                               errno: EBADF - Invalid file descriptor fd. */
      
      report_error("An error occurred while readdir.", NULL);
      return;
    }
    
    if (strcmp(dir_entry->d_name, CURRENT_DIR) && strcmp(dir_entry->d_name, PARENT_DIR)) {
    
      char path_new[strlen(dir_name)+strlen(dir_entry->d_name) + 2];      
      path_combine(dir_name, dir_entry->d_name, path_new);    
      
      errno = 0; /* Init. errno */
      long cur_location = telldir(path);      
      
      if (cur_location < 0 && errno != 0) { /* Error handling - Error handling - on error -> -1
                                                                errno: EBADF - Invalid directory stream descriptor path.*/
       
        report_error("An error occurred. Invalid directory stream descriptor path.", NULL);
        return;
      }
      
      errno = 0; /* Init. errno */
      if (closedir(path) < 0 && errno != 0) { /* Error handling - Error handling - on error -> -1
                                                                  errno: EBADF - Invalid directory stream descriptor path. */
    
        report_error("An error occurred. Invalid directory stream descriptor path.", NULL);
        return;
      }
      
      do_file(path_new, parms);
      
      errno = 0; /* Init. errno */
      if (!(path = opendir(dir_name)) && errno != 0) {  /* Error handling - Error handling - on error -> NULL 
                                                                            errno:  EACCES  - Permission denied. 
                                                                                    EBADF   - fd is not a valid file descriptor opened for reading. 
                                                                                    EMFILE  - Too many file descriptors in use by process. 
                                                                                    ENFILE  - Too many files are currently open in the system. 
                                                                                    ENOENT  - Directory does not exist, or name is an empty string. 
                                                                                    ENOMEM  - Insufficient memory to complete the operation. 
                                                                                    ENOTDIR - name is not a directory.  */
    
        report_error("An error occurred while opendir.", NULL);
        return;
      }
      
      seekdir(path, cur_location);  /* seekdir() returns no value. No error handing needed - see man page*/
    }
  }
  
  errno = 0; /* Init. errno */
  if (closedir(path) < 0 && errno != 0) { /* Error handling - Error handling - on error -> -1
                                                              errno: EBADF - Invalid directory stream descriptor path.*/
    
        report_error("An error occurred. Invalid directory stream descriptor path.", NULL);
        return;
  }
  
  return;
}

/**
 *\name path_combine
 *
 *\brief Returns an integer for each command.
 *
 * Combines path1 with path2 and return result in path_combined.
 *
 *\param path1 A char pointer that contains the command.
 *\param path2 A char pointer that contains the command.
 *\param path_combined A char pointer that contains the result of the function.
 *
 *\return Returns an integer that contains the length of the combined path.
 */
static int path_combine(const char *src_path1, const char *src_path2, char *dest) {
      
  int count = strlen(strcpy(dest, src_path1));  
  
  if (count && dest[count - 1] != '/') {       
    dest[count] =  '/';
  }
     
  if (count) {
      
    count = strlen(strcpy((dest[count] == '/' ? &dest[count + 1] : &dest[count]), src_path2));
  }
     
  return count;   
}

/**
 *\name basename
 *
 *\brief Returns an const char pointer that contains the base name.
 *
 * Own implementation.
 *
 *\param filename A char pointer that contains the full file name.
 *
 *\return Returns an const char pointer that contains the base name.
 */
static const char *basename(const char *filename) {

    if (filename == NULL) {
      
      report_error("An error occurred in function 'basename'. Parameter filename is NULL.", NULL);
      force_write_user_space();
    }
    
    char *base_name = strrchr(filename, '/');

    return !base_name ? filename :  ++base_name; 
}

/**
 *\name print_info
 *
 *\brief Prints a short info.
 *
 * Prints a short info.
 *
 *\return void
 */
static void print_info(void) {
	
  if (printf("myfind <file or directory> [ <aktion> ] ...\n") < 0) {  /* Error handling - Error handling - on error -> -1
                                                                                          errno: No error number */
                                                                                          
    report_error("An error occurred while print info.", NULL);
    /* 
     * ### FB: GRP14: print_info ist void function und sollte keinen return value haben, 
     *                  return EXIT_FAILURE hat value 1 -> besser exit(EXIT_FAILURE) 
     *                  wie bspw. force_write_user_space() da exit() void function ist 
     */  
    return EXIT_FAILURE;    
  }
}

/**
 *\name force_write_user_space
 *
 *\brief Free resources and forces output.
 *
 * Free resources and forces output.
 *
 *\param
 *\return
 *
 */
static void force_write_user_space(void) {

	if (fflush(stdout) == EOF) {  /* Error handling - Error handling - on error -> EOF and errno is set
                                                                     errno: EBADF - Stream is not an open stream, or is not open for writing. */
    
    report_error("An error occurred while force_write_user_space().", NULL);
  }	
  exit(EXIT_FAILURE);
}

/**
 *\name report_error
 *
 *\brief Print error
 *
 * If error number is set print details
 *
 *\param msg A char pointer that contains the error message
 *\param arg A char pointer that contains an additional argument
 *
 *\return void     
 *
 */
static void report_error(const char *msg, const char *arg)
{					
  if (fprintf(stderr,"%s: %s%s%s%s%s%s%s\n", PROG_NAME,
                                    (errno != 0 ? "Error: " : ""),
                                    (errno != 0 ? strerror(errno) : ""),
                                    (errno != 0 ? ". Message: " : ""),
                                     msg,
                                     arg != NULL ? "'" : "",
                                     arg != NULL ? arg : "",
                                     arg != NULL ? "'" : "") < 0) {  /* Error handling - Error handling - on error -> -1
                                                                      errno: No error number */
                                                                      
    report_error("An error occurred while print error.", NULL);
    /* 
     * ### FB: GRP14: do_file ist void function und sollte keinen return value haben, 
     *                return EXIT_FAILURE hat value 1 -> besser exit(EXIT_FAILURE) 
     *                  wie bspw. force_write_user_space() da exit() void function ist 
     */  
    return EXIT_FAILURE;  
  }
}

/**
 *\name parameter_print
 *
 *\brief Used for command -print
 *
 * This function prints the full file name to screen. If no file name is defined or parameter is NULL, nothing to print.
 *
 *\param file_name A const char pointer that contains the file name to print. If file name is NULL nothing to print.
 *
 *\return Returns an integer that contains EXIT_SUCCESS if no error occurred; otherwise EXIT_FAILURE.
 *\retval 0 EXIT_SUCCESS - No error occurred.
 *\retval 1 EXIT_FAILURE - An error occurred
 */
int param_print(const char *file_name) {
  
  if (file_name == NULL) {     /* if NULL nothing to print */
    
    return EXIT_SUCCESS;
  }
    
  if (printf("%s\n", file_name) < 0) { /* Error handling - Error handling - on error -> -1
                                                                  errno: No error number */

    report_error("An error occurred while print filename.", NULL);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}

/**
 *\name param_ls
 *
 *\brief Used for parameter -ls. Shows filename details.
 *
 *\param file_name The filename to print
 *
 *\return Returns an integer that contains EXIT_SUCCESS if no error occurred; otherwise EXIT_FAILURE.
 *\retval 0 EXIT_SUCCESS - No error occurred.
 *\retval 1 EXIT_FAILURE - An error occurred
 */
int param_ls(const char *file_name, const struct stat *file_info) {
  
  if (file_name == NULL) {     /* if NULL nothing to print */
    
    return EXIT_SUCCESS;
  }
  
  if (file_info == NULL) {     /* if NULL error */
    
    report_error("An error occurred. Parameter file_info is NULL.", NULL);
		force_write_user_space();
  }
  
  errno = 0; /* Init. errno */
  struct passwd *usr = getpwuid(file_info->st_uid);   /* get password info */
    
  if(!usr && errno != 0) { /* Error handling - Error handling - on error -> NULL and errno is set
                                               errno:  EINTR  - A signal was caught.
                                                       EIO    - I/O error.
                                                       ENFILE - The maximum number of files was open already in the system.
                                                       ENOMEM - Insufficient memory to allocate passwd structure.
                                                       ERANGE - Insufficient buffer space supplied. */
		
    report_error("An error occurred while read user.", NULL);
		force_write_user_space();
	}
 
  errno = 0; /* Init. errno */
	struct group  *grp = getgrgid(file_info->st_gid);   /* get group info    */
	
  if(!grp && errno != 0) { /* Error handling - Error handling - on error -> NULL and errno is set
                                               errno:  EINTR  - A signal was caught.
                                                       EIO    - I/O error.
                                                       ENFILE - The maximum number of files was open already in the system.
                                                       ENOMEM - Insufficient memory to allocate group structure.
                                                       ERANGE - Insufficient buffer space supplied. */
		
    report_error("An error occurred while read group.", NULL);
		force_write_user_space();
	}

  /* Decode file type */
  char file_type = '-';                               /* init file type    */ 
  
  switch (file_info->st_mode & S_IFMT) {
    
    case S_IFSOCK:
      file_type = 's';  /* socket     */
      break;
    case S_IFLNK:
      file_type = 'l';  /* link       */
      break;
    case S_IFREG:
      file_type = '-';  /* regular    */
      break;
    case S_IFBLK:
      file_type = 'b';  /* block      */
      break;
    case S_IFDIR:
      file_type = 'd';  /* directory  */
      break;
    case S_IFCHR:
      file_type = 'c';  /* character  */
      break;
    case S_IFIFO:
      file_type = 'p';  /* FIFO       */
      break;
    default:
      file_type = '-';  /* unknown    */
  }
 
  /* Decode uid */
  char uid[UID_GID_BUFFER_SIZE];  
	if (sprintf(uid, "%u", file_info->st_uid) < 0) {
    
    report_error("An error occurred while set uid.", NULL);
    return EXIT_FAILURE;
  }  
 
  /* Decode gid */
  char gid[UID_GID_BUFFER_SIZE];
  if (sprintf(gid, "%u", file_info->st_gid) < 0) {
    
    report_error("An error occurred while set gid.", NULL);
    return EXIT_FAILURE;
  }
  
  /* Decode owner exec bit          */
  char own_exec = (file_info->st_mode & S_IXUSR) ? 
                    ((file_info->st_mode & S_ISUID) ? 's' : 'x') : 
                    ((file_info->st_mode & S_ISUID) ? 'S' : '-');
  
  /* Decode group exec bit          */
  char grp_exec = (file_info->st_mode & S_IXGRP) ? 
                    ((file_info->st_mode & S_ISGID) ? 's' : 'x') : 
                    ((file_info->st_mode & S_ISGID) ? 'S' : '-');
  
  /* Decode other exec (sticky) bit */
  char oth_exec = (file_info->st_mode & S_IXGRP) ? 
                    ((file_info->st_mode & S_ISVTX) ? 't' : 'x') : 
                    ((file_info->st_mode & S_ISVTX) ? 'T' : '-');
    
  /* Decode file time               */ 
  char *months[]    = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  
  struct tm *date_time = localtime(&file_info->st_mtime);

  if (date_time == NULL) {
    
    report_error("An error occurred while calc date_time.", NULL);
    return EXIT_FAILURE;
  }
  
 /* Decode file size */ /* if file type is block or char don't show file length */ 
  char file_size[FILE_SIZE_BUFFER_SIZE];

  if (file_info->st_size == 0 && (file_type == 'c' || file_type == 'b')) {
    
    file_size[0] = '\0';
    
  } else {
    
    if (sprintf(file_size, "%lld", (long long)file_info->st_size) < 0) {
    
      report_error("An error occurred while set uid.", NULL);
      return EXIT_FAILURE;
    } 
  }
  /* ### FB: GRP14: number of links wird hier als int ausgegeben (14.Argument) -> müsste eigentlich long unsigned int sein  */
  /* Print file details */                        
  if (printf("%6ld %4d %c%c%c%c%c%c%c%c%c%c %3d %-8s %-8s %-8s %3s %2d %02d:%02d %s%s", 
            file_info->st_ino,                                                                            /* the index number of file      */
            getenv("POSIXLY_CORRECT") ? (int)file_info->st_blocks : ((int)file_info->st_blocks + 1) / 2,  /* blocks for disk usage         */
            file_type,                                                                                    /* file type bit                 */
            (file_info->st_mode & S_IRUSR) ? 'r' : '-',                                                   /* owner read allowed bit        */
            (file_info->st_mode & S_IWUSR) ? 'w' : '-',                                                   /* owner write allowed bit       */
            own_exec,                                                                                     /* owner exec bit (SUID)         */
            (file_info->st_mode & S_IRGRP) ? 'r' : '-',                                                   /* group read allowed bit        */
            (file_info->st_mode & S_IWGRP) ? 'w' : '-',                                                   /* group write allowed bit       */
            grp_exec,                                                                                     /* group exec bit (SGID)         */
            (file_info->st_mode & S_IROTH) ? 'r' : '-',                                                   /* others read allowed bit       */
            (file_info->st_mode & S_IWOTH) ? 'w' : '-',                                                   /* others write allowed bit      */
            oth_exec,                                                                                     /* others exec (sticky) bit      */
            file_info->st_nlink,                                                                          /* number of links               */
            usr ? usr->pw_name : uid,                                                                     /* user name                     */
            grp ? grp->gr_name : gid,                                                                     /* group name                    */
            &file_size[0],                                                                                /* file size                     */
            months[date_time->tm_mon],                                                                    /* date and time [month] of file */
            date_time->tm_mday,                                                                           /* date and time [day] of file   */
            date_time->tm_hour,                                                                           /* date and time [hour] of file  */
            date_time->tm_min,                                                                            /* date and time [min] of file   */
            file_name,                                                                                    /* filename                      */
            S_ISLNK(file_info->st_mode) ? "" : "\n")                                                      /* link path                     */
       < 0) {   /* Error handling - Error handling - on error -> -1
                                    errno: No error number */

    report_error("An error occurred while print filename details.", NULL);
    return EXIT_FAILURE;
  }
  
  /* Decode link path */
  if (S_ISLNK(file_info->st_mode)) {  
         
    char link[file_info->st_size + 5]; strcpy(link, " -> "); link[file_info->st_size + 4] = '\0';         /* " -> " + buffer + '\0'       */

    errno = 0; /* Init. errno */
    if (readlink(file_name, &link[4], sizeof(link)) < 0) { /* Error handling - Error handling - on error -> -1 and errno is set
                                                                               errno: EACCES  - Search permission is denied for a component of the path prefix. 
                                                                                      EFAULT  - buf extends outside the process’s allocated address space. 
                                                                                      EINVAL  - bufsiz is not positive.
                                                                                      EINVAL  - The named file is not a symbolic link. 
                                                                                      EIO     - An I/O error occurred while reading from the file system. 
                                                                                      ELOOP   - Too many symbolic links were encountered in translating the pathname.
                                                                                      ENAMETOOLONG - A pathname, or a component of a pathname, was too long.
                                                                                      ENOENT  - The named file does not exist.
                                                                                      ENOMEM  - Insufficient kernel memory was available. 
                                                                                      ENOTDIR - A component of the path prefix is not a directory. */
   
      report_error("An error occurred while readlink.", NULL);
      return EXIT_FAILURE;
    }
    
    /* print link */
    if( printf("%s\n", &link[0]) < 0) { /* Error handling - Error handling - on error -> -1
                                                            errno: No error number */
      
      report_error("An error occurred while print file link details.", NULL);
      return EXIT_FAILURE;
    }
  }
    
  return EXIT_SUCCESS;
}

/**
 *\name check_if_usr
 *
 *\brief checks if user is the fileowner of a file 
 *
 * 
 *
 *\param file A char pointer to a struct stat of the file to check
 *\param usr A char pointer to a string that contains the username or uid.
 *
 *
 *\return int
 *        Returns 0 if user is fileowner 
 *        Returns 1 if user is not fileowner 
 */
static int check_if_usr(const struct stat *file_info, const char *usr){
	const struct passwd *pwd_entry;
	int uid;
	
	errno = 0;
	pwd_entry = getpwnam(usr);	
	
	if(pwd_entry == NULL){
		/*check if usr is a valid uid */
		uid = string_to_id(usr);
		if(uid == -1){ 
			fprintf(stderr,"%s: `%s' is not the name of a known user\n",PROG_NAME,usr);
			exit(EXIT_FAILURE);
		}

		if(uid == -2){ 
			fprintf(stderr,"%s: %s: Numerical result out of range\n",PROG_NAME,usr);
			exit(EXIT_FAILURE);
		}
		
		errno = 0;
		pwd_entry = getpwuid(uid);
		if(pwd_entry == NULL){	
			fprintf(stderr,"%s: `%s' is not the name of a known user\n",PROG_NAME,usr);
			exit(EXIT_FAILURE);
		}
	}

	if(pwd_entry->pw_uid == file_info->st_uid){ //if usr is fileowner 
	    return 0; 
		}
	
	//if fileowner not usr
	return 1;
}

/**
 *\name check_if_grp
 *
 *\brief checks if file belongs to group 
 *
 * 
 *
 *\param file A pointer to a struct stat of the file to check
 *\param grp A char pointer to a string that contains the groupname or gid.
 *
 *
 *\return int
 *        Returns 0 if file belongs to group 
 *        Returns 1 if file does not belong to group 
 */
static int check_if_grp(const struct stat *file_info, const char *grp){
	const struct group *grp_entry;
	int gid;
	
	errno = 0;
	grp_entry = getgrnam(grp);	
	
	if(grp_entry == NULL){
		gid = string_to_id(grp);
		if(gid == -1){ 
			fprintf(stderr,"%s: `%s' is not the name of a known group\n", PROG_NAME, grp);
			exit(EXIT_FAILURE);
		}

		if(gid == -2){ 
			fprintf(stderr,"%s: %s: Numerical result out of range\n", PROG_NAME, grp);
			exit(EXIT_FAILURE);
		}
		
		errno = 0;
		grp_entry = getgrgid(gid);
		if(grp_entry == NULL){	
			fprintf(stderr,"%s: `%s' is not the name of a known group\n",PROG_NAME,grp);
			exit(EXIT_FAILURE);
		}
	}

	if(grp_entry->gr_gid == file_info->st_gid){  
	    return 0; 
		}
	
	return 1;
}

/**
 *\name string_to_id
 *
 *\brief converts a uid or gid given as string to an integer id 
 *
 * 
 *
 *\param id_str A char pointer that contains the directory name.
 * 
 *
 *\return int 
 *        Returns an id
 *        Returns -1 if id_str not only contains numbers
 *				Returns -2 if id_str is larger than 2147483647 
 */
static int string_to_id(const char *id_str){
	long long id;
  char *endptr = NULL;
   
	id = strtoll(id_str,&endptr, 10);
     
	/*check if in id_str are other chars than numbers*/
    	if(endptr != (id_str + strlen(id_str) )){
     		return -1;
   	}

  	/*check if nbr  < 2147483647 */
   	if(id > 2147483647 ){
     		return -2;
   	}
   return id;
}

/**
 *\name check_if_name
 *
 *\brief The check_if_name() function checks whether the string argument matches the pattern argument
 *
 * The check_if_name() function checks whether the string argument matches the pattern argument
 *
 *\param file A char pointer to a string that contains the file name.
 *\param filter A char pointer to a string that contains the pattern argument.
 *
 *\return Returns an integer that contains the value EXIT_SUCCESS if found; otherwise EXIT_FAILURE.
 *\retval EXIT_SUCCESS
 *\retval EXIT_FAILURE
 */
static int check_if_name(const char *file, const char *pattern) {
	 
  if (!file) {                   /* Checks if file name is NULL */
    
    report_error("An error occurred. Filename in function check_if_name is NULL.", NULL);
    force_write_user_space();
  }
  
  if (!pattern) {                /* Checks if pattern is invalid */
    
    report_error("An error occurred. Pattern in function check_if_name is NULL.", NULL);
    force_write_user_space();
  }
      
  int match = fnmatch(pattern, basename(file) , FNM_NOESCAPE);

  if (match != FNM_NOMATCH && match != 0) {     /* Error handling - Error handling - on error -> match != FNM_NOMATCH && match != 0
                                                                    errno: Not used */
    
    report_error("An error occurred while match file name.", NULL);
    force_write_user_space();
  }
    
	return (!match ? EXIT_SUCCESS : EXIT_FAILURE);
}

/**
 *\name check_if_path
 *
 *\brief The check_if_path() function checks whether the string argument matches the pattern argument
 *
 * The check_if_path() function checks whether the string argument matches the pattern argument
 *
 *\param file A char pointer to a string that contains the path name.
 *\param filter A char pointer to a string that contains the pattern argument.
 *
 *\return Returns an integer that contains the value EXIT_SUCCESS if found; otherwise EXIT_FAILURE.
 *\retval EXIT_SUCCESS
 *\retval EXIT_FAILURE
 */
static int check_if_path(const char *path, const char *pattern) {
	 
  
  if (!path) {                   /* Checks if path name is NULL */
   
    report_error("An error occurred in check_if_name(). Parameter path is NULL.", NULL);
    force_write_user_space();
  }
  
  if (!pattern) {                   /* Checks if pattern is invalid */
    
    report_error("An error occurred in check_if_name(). Parameter pattern is NULL.", NULL);
    force_write_user_space();
  }
    
  int match = fnmatch(pattern, path, FNM_NOESCAPE);
    
  if (match != FNM_NOMATCH && match != 0) {     /* Error handling - Error handling - on error -> match != FNM_NOMATCH && match != 0
                                                                    errno: Not used */
 
    report_error("An error occurred while match path name.", NULL);
    force_write_user_space();
  }
    
	return (!match ? EXIT_SUCCESS : EXIT_FAILURE);
}

/**
 *\name check_if_type
 *
 *\brief Compares file name properties.
 * 
 * Compares file name properties.
 *
 *\param file_info A struct pointer to the file properties.
 *\param flags A const char pointer that contains the property flag.
 *
 *\return Return EXIT_SUCCESS (0) if flag is set; otherwise EXIT_FAILURE (1) is set.
 *\retval EXIT_SUCCESS (0)
 *\retval EXIT FAILURE (1)
 */
static int check_if_type(const struct stat *file_info, const char *flag) {

  switch (file_info->st_mode & S_IFMT) {  /* Mask file mode */
    case S_IFBLK: 
      return !(*flag == 'b');             /* Block special file */
    case S_IFCHR: 
      return !(*flag == 'c');             /* Character special file */
    case S_IFDIR:
      return !(*flag == 'd');	            /* Directory */
    case S_IFIFO:
      return !(*flag == 'p'); 	          /* Pipe */
    case S_IFREG: 
      return !(*flag == 'f');	            /* Reg. file name */
    case S_IFLNK: 
      return !(*flag == 'l');             /* Symbolic link */
    case S_IFSOCK: 
      return !(*flag == 's');	            /* Socket */
    default:
      return EXIT_FAILURE;
  }
}

/**
 *\name check_if_no_grp
 *
 *\brief checks if file belong to no valid group
 *
 * 
 *
 *\param  A pointer to a struct stat of the file to check
 *
 *
 *\return int
 *        Returns 0 if group of the file exists in group-database
 *        Returns 1 if group of the file does not exist
 */

static int check_if_no_grp(const struct stat *file_info){
  
  errno = 0;
  if(getgrgid(file_info->st_gid) == NULL){
    if(errno != 0){  
      report_error("An error occurred while checking nogroup.", NULL);
    }
    return 1;                 // falls ungültige gruppe
  }
  
	return 0;
}

/**
 *\name check_if_no_usr
 *
 *\brief checks if file belongs to no valid user
 *
 * 
 *
 *\param  A pointer to a struct stat of the file to check
 *
 *
 *\return int
 *        Returns 0 if user of the file exists in passwd
 *        Returns 1 if user of the file does not exist
 */

static int check_if_no_usr(const struct stat *file_info){
  
  errno = 0;
  if(getpwuid(file_info->st_uid) == NULL){
    if(errno != 0){  
      report_error("An error occurred while checking nouser.", NULL);
    }
    return 1;                 // falls ungültiger user
  }
  
	return 0;
}
