/**
 * @file main.c
 *
 * Betriebssysteme: Beispiel 1 -> myfind
 *
 * @author Sabrina Mehlmann  <ic17b060@technikum-wien.at>
 * @author Gregor Mersich    <ic17b020@technikum-wien.at>
 * @author Jan Reichetzeder  <ic17b018@technikum-wien.at>
 *
 * @version 1.0
 *
 */

/* ------------------------------------------  includes ----------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fnmatch.h>
#include <time.h>
#include <limits.h>
#include <grp.h>
#include <locale.h>
#include <langinfo.h>
#include <stdint.h>

/* ------------------------------  global variables -------------------------- */

static const char *prog_name;
static int pdim;

/* ------------------------------  function prototypes ----------------------- */

static void pparameter(const char **arguments, const char ** parameters, int argument_count);
/*static void errmsg(char *file_name);*/

static void do_dir (const char * dirname, const char * const *parms);
static void do_file (const char * file_name, const char *const *parms, const char *curname);

static int check_user(const char *user, struct stat current_entry);
static void print_ls(const char *filename, const struct stat sb);
static int nouser(const struct stat sb);
static int check_namepath (const char *pattern, const char *string, const char *printpath);
static int check_type(const char *current_path, const char *type, struct stat current_entry);

/**
 *\name main
 *
 *\brief Main function
 *
 *\param argc number of passed arguments
 *\param argv pointer to those passed arguments
 *exit(EXIT_FAILURE)
 *\return Returns EXIT_SUCCESS if successfully; otherwise EXIT_FAILURE.
 *\retval EXIT_SUCCESS
 *\retval EXIT_FAILURE
 */

int main(int argc, const char * argv[]) {
    
    /* Initialize prog_name with argv[0] */
    prog_name = argv[0];
    
    /* Initialize parameter-dimension pdim and check if enough parameters were passed, else EXIT_FAILURE */
    pdim = argc - 1;
    if(pdim < 1)
    {
        fprintf(stderr, "\nERROR: %s: Not enough parameters\n", prog_name);
        exit(EXIT_FAILURE);
    }
    
    /* Pass arguments to parms for do_file */
    const char **parms;
    parms = malloc(sizeof(char*) * (argc -1));
    if(parms == NULL)
    {
        fprintf(stderr, "\nERROR: %s: %s\n", prog_name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    pparameter(argv, parms, argc);
    
    /* Pass filepath from parms to path */
    char path[strlen(parms[0])];
    strcpy(path, parms[0]);
    
    //    aufrufen der Funktion do_dir mit dem uebergebenen Pfad und parms (derzeit unwichtig)
    do_dir(path, parms);
    
    /* Free allocated memory */
    free(parms);
    
    return EXIT_SUCCESS;
    
}

/**
 *\name pparameter
 *
 *\brief passes arguments of argv to parms except for programm name
 *
 *\param arguments is a pointer to the arguments given to the program
 *\param parameters is a pointer to the new arguments
 *\param argument_count is the number of arguments given to the program
 *
 *\return void function does not have a return value
 */

static void pparameter(const char ** arguments, const char ** parameters, int argument_count)
{
    int i;
    
    for(i = 0; i < argument_count; i++)
    {
        parameters[i] = arguments[i + 1];
        
    }
}

/**
 *\name do_dir
 *
 *\brief
 *
 *\param file_name is the path of the starting point
 *\param parms
 *\param pdim
 *
 *\return void function does not have a return value
 */

static void do_dir (const char * dir_name, const char * const *parms)
{
    /* opens directory stream for the current directory */
    DIR *curdir = opendir(dir_name);
    
    /* checks if opdendir() did not result in an error */
    if (errno != 0)
    {
        fprintf(stderr, "\nERROR: %s: %s - %s\n", prog_name, strerror(errno), dir_name);
        errno = 0;
    }
    
    struct dirent *curstruct;
    
    /* weist curstruct das aktuelle Ergebnis aus readdir() zu; solange readdir() keinen NULL-Pointer ausgibt wird der Anweisungsblock ausgefuehrt */
    while ((curstruct = readdir(curdir)))
        
    {
        /* checks if readdir() did not result in an error */
        if (errno != 0)
        {
            fprintf(stderr, "\nERROR: %s:  %s - %s\n", prog_name, strerror(errno), dir_name);
            errno = 0;
        }
        
        /* skips entrys named "." or ".." */
        if ((strcmp(curstruct->d_name, ".") == 0) || (strcmp(curstruct->d_name, "..") == 0) || (strcmp(curstruct->d_name, ".DS_Store") == 0))  continue;
        
        /*        char newpath[] wird erstellt; die Dimenson wird mit der Laenge des bisherigen Pfades, der Laenge des Namens des aktuellen Eintrags plus einem weiteren Feld angegeben */
        char newpath[(strlen(dir_name))+(strlen(curstruct->d_name))+1];
        
        /*        Dieses Konstrukt fuegt den aktuellen Pfad + "/" + den aktuell bearbeiteten Eintrag zusammen, sodass in newpath der neue, aktuellste Pfad gespeichert ist */
        strcpy(newpath, dir_name);
        strcat(newpath, "/");
        strcat(newpath, curstruct->d_name);
        
        /*        do_file() wird mit dem aktuellsten Pfad (newpath) aufgerufen */
        do_file(newpath, parms, curstruct->d_name);
        
    }
    
    closedir(curdir);
    
}

/**
 *\name do_file
 *
 *\brief
 *
 *\param file_name
 *\param parms
 *\curname
 *\pdim
 *
 *\return void function does not have a return value
 */

static void do_file (const char * file_name, const char *const *parms, const char *curname)
{
    //Struct vom Typ "stat" wird erstellt und mit dem Output von lstat() befuellt
    struct stat curentry;
    
    lstat(file_name, &curentry);
    
    int i = 0;
    int print = 0;
    
    for(i=1; i<pdim; i++)
    {
        if(strcmp(parms[i],"-name")==0)
        {
            if((i+1)>(pdim-1))
            {
                fprintf(stderr, "ERROR: %s: \"%s\" requires attribute.", prog_name, parms[i]);
                exit(1);
            }
            
            if (check_namepath(parms[i+1], curname, file_name))
            {
                print = 1;
                i++;
            }
            else {print = 0; break;}
        }
        
        else if(strcmp(parms[i],"-path")==0)
        {
            if((i+1)>(pdim-1))
            {
                fprintf(stderr, "ERROR: %s: \"%s\" requires attribute.", prog_name, parms[i]);
                exit(EXIT_FAILURE);
            }
            
            if (check_namepath(parms[i+1], file_name, file_name))
            {
                print = 1;
                i++;
            }
            else {print = 0; break;}
        }
        
        else if(strcmp(parms[i],"-user")==0)
        {
            if((i+1)>(pdim-1))
            {
                fprintf(stderr, "ERROR: %s: \"%s\" requires attribute.", prog_name, parms[i]);
                exit(EXIT_FAILURE);
            }
            
            if (check_user(parms[i+1], curentry))
            {
                print = 1;
                i++;
            }
            else {print = 0; break;}
        }
        
        else if(strcmp(parms[i],"-nouser")==0)
        {
            if (nouser(curentry))
            {
                print = 1;
            }
            else {print = 0; break;}
        }
        
        else if(strcmp(parms[i],"-type")==0)
        {
            if((i+1)>(pdim-1))
            {
                fprintf(stderr, "ERROR: %s: \"%s\" requires attribute.", prog_name, parms[i]);
                exit(EXIT_FAILURE);
            }
            
            if (check_type(file_name, parms[i+1], curentry))
            {
                print = 1;
                i++;
            }
            else {print = 0; break;}
        }
        
        else if(strcmp(parms[i],"-ls")==0)
        {
            print_ls(file_name, curentry);
            print = 0;
        }
        
        else if(strcmp(parms[i], "-print")==0)
        {
            printf("%s\n", file_name);
            print = 0;
        }
        
        else
        {
            printf("Unbekannter Parameter \"%s\"! Programm wurde beendet.\n", parms[i]);
            exit(EXIT_FAILURE);
        }
    }
    
    if(print || (pdim == 1)) printf("%s\n", file_name);
    
    /* checks if current entry is a directory, if yes do_dir is called */
    if(S_ISDIR(curentry.st_mode)) do_dir(file_name, parms);
    
}

/**
 *\name NOCH ZU BEARBEITEN!!!
 *
 *\brief Errormessage
 *
 *\param
 *\param
 *
 *\return
 *\retval
 *\retval
 */

/*
 static void errmsg(char *file_name)
 
 fprintf(stderr, "%s: %s\t%s\n", prog_name, strerror(errno), file_name);
 }
 */



/**
 *\name
 *
 *\brief
 *
 *\param
 *\param
 *
 *\return
 *\retval
 *\retval
 */
static int check_user(const char *user, struct stat current_entry)
{
    unsigned long int uid;
    char *ptemp;
    int x = 0;
    
    struct passwd *popt = getpwnam(user);
    
    if(popt != NULL)
    {
        if(popt->pw_uid == current_entry.st_uid) x = 1;
    }
    else
    {
        uid = strtoul(user, &ptemp, 10);
        if(*ptemp == '\0')
        {
            if(uid == current_entry.st_uid) x = 1;
            if(getpwuid(uid) == NULL)
            {
                fprintf(stderr, "ERROR: %s: FEHLER\n", prog_name);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    return x;
}

/**
 *\name
 *
 *\brief
 *
 *\param
 *\param
 *
 *\return
 *\retval
 *\retval
 */

static void print_ls(const char *filename, const struct stat sb) {
    
    struct passwd  *pwd;
    struct group   *grp;
    
    char * user;
    char * group;
    char permis[30];
    
    strcpy(permis, (S_ISDIR(sb.st_mode)) ? "d" : "-");
    strcat(permis, (sb.st_mode & S_IRUSR) ? "r" : "-");
    strcat(permis, (sb.st_mode & S_IWUSR) ? "w" : "-");
    strcat(permis, (sb.st_mode & S_IXUSR) ? "x" : "-");
    
    strcat(permis, (sb.st_mode & S_IRGRP) ? "r" : "-");
    strcat(permis, (sb.st_mode & S_IWGRP) ? "w" : "-");
    strcat(permis, (sb.st_mode & S_IXGRP) ? "x" : "-");
    strcat(permis, (sb.st_mode & S_IROTH) ? "r" : "-");
    strcat(permis, (sb.st_mode & S_IWOTH) ? "w" : "-");
    strcat(permis, (sb.st_mode & S_IXOTH) ? "x" : "-");
    
    
    grp = getgrgid(sb.st_gid);
    pwd = getpwuid(sb.st_uid);
    
    
    if (pwd == NULL) {
        user = alloca(10);
        snprintf(user, 10, "%u", sb.st_uid);
    } else {
        user = pwd->pw_name;
    }
    
    
    if (grp == NULL) {
        group = alloca(10);
        snprintf(group, 10, "%u", sb.st_gid);
    } else {
        group = grp->gr_name;
    }
    
    
    char *p;
    p = ctime(&sb.st_ctime);
    p += 3;
    
    //----------------sym-link vorarbeiten----------------------------------//
    
    char *symlink = NULL;
    ssize_t r, bsize;
    bsize = sb.st_size +1;
    
    if (S_ISLNK(sb.st_mode)!= 0) {
        if (sb.st_size == 0)
            bsize = PATH_MAX;
        symlink = malloc(sizeof(char) * bsize);
        if (symlink == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        
        
        while ((r = readlink(filename, symlink, bsize)) > 1 && (r > bsize)) {
            bsize *= 2;
            if ((symlink = realloc(symlink, sizeof(char) * bsize)) == NULL) {
                fprintf(stderr,"Not enough memory to continue\n");
                exit(EXIT_FAILURE);
            }
            
        }
        if (r == -1) {
            perror("readlink");
            exit(EXIT_FAILURE);
        }
        
        
        symlink[r] = '\0';
    }
    
    //--------------------------------------------------//
    
    
    
    printf("   %ld %2lld %s %4d  %s %s %5jd %.13s %s %s %s \n",  sb.st_ino,  (long long) sb.st_blocks, permis, sb.st_nlink,
           user, group,
           (intmax_t)sb.st_size, p, filename, ((S_ISLNK(sb.st_mode)!= 0) ? "->" : ""),
           ((S_ISLNK(sb.st_mode)!= 0)? symlink : ""));
}

//Funktion überprüft ob es einen User gibt der mit der numerischen User-ID eines files übereinstimmt

/**
 *\name nouser
 *
 *\brief
 *
 *\param
 *\param
 *
 *\return
 *\retval
 *\retval
 */

static int nouser(const struct stat sb) {
    
    struct passwd  *pwd;
    pwd = getpwuid(sb.st_uid);
    
    if ((pwd = getpwuid(sb.st_uid)) == NULL) return 1;
    else return 0;
}

/**
 *\name check_type
 *
 *\brief
 *
 *\param current_path
 *\param type
 *\param current_entry
 *
 *\return
 *\retval
 *\retval
 */
static int check_type(const char *current_path, const char *type, struct stat current_entry)
{
    int x = 0;
    
    if(strlen(type)!= 1)
    {
        fprintf(stderr, "\nERROR: %s: Argument is too long - %s\n", prog_name, current_path);
    }
    else
    {
        switch(type[0])
        {
            case 'b': if(S_ISBLK(current_entry.st_mode)) x = 1; break;
            case 'c': if(S_ISCHR(current_entry.st_mode)) x = 1; break;
            case 'd': if(S_ISDIR(current_entry.st_mode)) x = 1; break;
            case 'p': if(S_ISFIFO(current_entry.st_mode)) x = 1; break;
            case 'f': if(S_ISREG(current_entry.st_mode)) x = 1; break;
            case 'l': if(S_ISLNK(current_entry.st_mode)) x = 1; break;
            case 's': if(S_ISSOCK(current_entry.st_mode)) x = 1; break;
            default: fprintf(stderr,"\nERROR: %s:  %s - %s\n", prog_name, strerror(errno), current_path); exit(EXIT_FAILURE);
        }
    }
    
    return x;
}

/**
 *\name
 *
 *\brief
 *
 *\param
 *\param
 *
 *\return
 *\retval
 *\retval
 */
static int check_namepath (const char *pattern, const char *string, const char *printpath)
{
    int x = fnmatch(pattern, string, FNM_NOESCAPE);
    
    //  Errorcheck fuer fnmatch()
    if (errno != 0)
    {
        fprintf(stderr, "\nERROR: %s:  %s - %s\n", prog_name, strerror(errno), printpath);
        errno = 0;
    }
    
    if (x == 0) return 1;
    else return 0;
}

/*
 * ======================================================================= EOF =========
 */
