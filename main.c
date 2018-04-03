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


// folgende "includes" sollten hinzugefügt werden, um die Funktionsweise
// von print_ls und nouser (siehe prototypes) zu gewährleisten

// ----------------------
#include <time.h>
#include <limits.h>
#include <grp.h>
#include <locale.h>
#include <langinfo.h>
#include <stdint.h>
// ----------------------


static void pparameter(const char **arguments, const char ** parameters, int argument_count);

static void do_dir (const char * file_name, const char * const *parms);
static void do_file (const char * file_name, const char *const *parms, const char *curname);

static void errmsg(int error_number, char *file_name);

// Funktionen zum Checken der Parameter
static void check_user_name(char *current_path, const char *user, struct stat current_entry);
static void check_uid(char *current_path, int user_id, struct stat current_entry);

// Funktionen für die Aktionen "-ls" und "-nouser"
// Diese sollten nach entsprechendem Parametervergleich in do_file aufgerufen werden
// Der Aufruf von print_ls sollte in der Form: print_ls(filename, curentry);  erfolgen
// Der Aufruf von nouser sollte in der Form: nouser(filename, curentry);  erfolgen

static void print_ls(const char *filename, const struct stat sb);
static void nouser(const char *filename, const struct stat sb);

static void check_name (char *pattern, char *string, char *printpath);

static void check_type(char *current_path, const char *type, struct stat current_entry);

int main(int argc, const char * argv[]) {
    
    //Deklarierung, Speicherallokierung und Initialisierung von parms exklusive Programmname -> ab Pfad
    const char **parms;
    parms = malloc(sizeof(char*) * (argc -1));
    pparameter(argv, parms, argc);
    
    //    char path[] wird definiert und danach wird parms (uebergebener Pfad) hineinkopiert
    char path[strlen(parms[0])];
    strcpy(path, parms[0]);
    
    //    aufrufen der Funktion do_dir mit dem uebergebenen Pfad und parms (derzeit unwichtig)
    do_dir(path, parms);
    
    //Freigeben von allokiertem Speicher
    free(parms);
    
    return 0;
}

static void pparameter(const char ** arguments, const char ** parameters, int argument_count)
{
    int i;
    
    for(i = 0; i < argument_count; i++)
    {
        parameters[i] = arguments[i + 1];
        
    }
}

static void do_dir (const char * file_name, const char * const *parms)
{
    //    oeffnet den Directory Stream fuer das aktuelle Verzeichnis
    DIR *curdir = opendir(file_name);
    
    //    checkt, ob opendir() fehlerlos aufgerufen wurde
    if (errno != 0)
    {
        printf("\nERROR: %s - %s\n", strerror(errno), file_name);
        errno = 0;
    }
    
    //    erstellt ein struct dirent in dem die Rueckgabe der Funktion readdir() gespeichert werden kann
    struct dirent *curstruct;
    
    //        weist curstruct das aktuelle Ergebnis aus readdir() zu; solange readdir() keinen NULL-Pointer ausgibt wird der Anweisungsblock ausgefuehrt
    while ((curstruct = readdir(curdir)))
        
    {
        //    checkt, ob readdir() fehlerlos aufgerufen wurde
        if (errno != 0)
        {
            printf("\nERROR: %s - %s\n", strerror(errno), file_name);
            errno = 0;
        }
        
        //        uebergeht alle Eintraege die "." oder ".." heissen, da diese zu einem Fehler beim ausfuehren fuehren; uebergeht auch ".DS_Store" -> nur in OSX (Mac) notwendig
        if ((strcmp(curstruct->d_name, ".") == 0) || (strcmp(curstruct->d_name, "..") == 0) || (strcmp(curstruct->d_name, ".DS_Store") == 0))  continue;
        
        //        char newpath[] wird erstellt; die Dimenson wird mit der Laenge des bisherigen Pfades, der Laenge des Namens des aktuellen Eintrags plus einem weiteren Feld angegeben
        char newpath[(strlen(file_name))+(strlen(curstruct->d_name))+1];
        
        //        Dieses Konstrukt fuegt den aktuellen Pfad + "/" + den aktuell bearbeiteten Eintrag zusammen, sodass in newpath der neue, aktuellste Pfad gespeichert ist
        strcpy(newpath, file_name);
        strcat(newpath, "/");
        strcat(newpath, curstruct->d_name);
        
        //        do_file() wird mit dem aktuellsten Pfad (newpath) aufgerufen
        do_file(newpath, parms, curstruct->d_name);
        
    }
    
    closedir(curdir);
    
}


static void do_file (const char * file_name, const char *const *parms, const char *curname)
{
    //    wird derzeit nur zum pruefen der Funktion verwendet; diese "printf" wird in weiterer Folge geloescht werden, da die -print Aktion bereits besteht
    printf("%s\n", file_name);
    
    struct stat curentry;
    
    lstat(file_name, &curentry);
    
    //    ueberprueft, ob es sich bei dem aktuellen Eintrag um ein Directory handelt; falls ja wird do_dir() mit dem aktuellen Pfad aufgerufen
    if(S_ISDIR(curentry.st_mode)) do_dir(file_name, parms);
    
}

//Funktion gibt Standard-error von errno aus und wo error aufgetreten ist (path)
static void errmsg(int error_number, char *file_name)
{
    fprintf(stderr, "myfind: %s\t%s\n", strerror(error_number), file_name);
}

// Funktion printet Fehler wenn Username nicht in "/etc/passwd" gefunden wird,
// printet Pfad, wenn Parameter-User mit User von current_entry übereinstimmen
static void check_user_name(char *current_path, const char *user, struct stat current_entry)
{
    struct passwd *popt;
    if(getpwnam(user)!=0)
    {
        popt = getpwnam(user);
        if(popt->pw_uid == current_entry.st_uid)
        {
            printf("%s\n", current_path);
        }
    }
    else errmsg(errno, path);
}

// Funktion printet Fehler wenn User ID nicht in "/etc/passwd" gefunden wird,
// printet Pfad, wenn Parameter-User ID mit User ID von current_entry übereinstimmen
static void check_uid(char *current_path, char *userID, struct stat current_entry)
{
    struct passwd *popt;
    int user_id = atoi(userID);
    
    if(getpwuid(user_id)!=0)
    {
        popt = getpwuid(user_id);
        if(popt->pw_uid == current_entry.st_uid)
        {
            printf("%s\n", current_path);
        }
    }
    else errmsg(errno, path);
}

// Funktionen für die Aktionen "-ls" und "-nouser"
// Diese sollten nach entsprechendem Parametervergleich in do_file aufgerufen werden
// Der Aufruf von print_ls sollte in der Form: print_ls(filename, curentry);  erfolgen
// Der Aufruf von nouser sollte in der Form: nouser(filename, curentry);  erfolgen

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
    
    
    
    printf("   %llu %2lld %s %4d  %s %s %5jd %.13s %s %s %s \n",  sb.st_ino,  (long long) sb.st_blocks, permis, sb.st_nlink,  
           user, group,   
           (intmax_t)sb.st_size, p, filename, ((S_ISLNK(sb.st_mode)!= 0) ? "->" : ""),
           ((S_ISLNK(sb.st_mode)!= 0)? symlink : ""));  
}

//Funktion überprüft ob es einen User gibt der mit der numerischen User-ID eines files übereinstimmt

static void nouser(const char *filename, const struct stat sb) {
    
    struct passwd  *pwd;
    pwd = getpwuid(sb.st_uid);
    
    if ((pwd = getpwuid(sb.st_uid)) == NULL)
        printf("Kein User entspricht der numerischen User-ID %d des Files: %s\n",sb.st_uid, filename);
}



// Funktion überprüft Länge der Parameters zu -type,
// Überprüfung ob der Dateityp vom current_entry dem eingegebenen type entspricht
// wenn ja, dann printf vom current_path
static void check_type(char *current_path, const char *type, struct stat current_entry)
{
    if(strlen(type)!= 1) printf("UNGÜLTIGE EINGABE");
    else
    {
        switch(type[0])
        {
            case 'b': if(S_ISBLK(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 'c': if(S_ISCHR(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 'd': if(S_ISDIR(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 'p': if(S_ISFIFO(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 'f': if(S_ISREG(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 'l': if(S_ISLNK(current_entry.st_mode)) printf("%s\n", current_path); break;
            case 's': if(S_ISSOCK(current_entry.st_mode)) printf("%s\n", current_path); break;
            default: printf("Dateityp existiert nicht"); break;
        }
    }
}

//Funktion ueberprueft, ob der Name des derzeitigen Eintrages mit dem Muster, welches mit "-name" uebergeben wurde, zusammenpasst; falls ja, wird der aktuelle Pfad ausgegeben
static void check_name (char *pattern, char *string, char *printpath)
{
    if (fnmatch(pattern, string, FNM_NOESCAPE) == 0) printf("%s\n", printpath);
    
    //  Errorcheck fuer fnmatch()
    if (errno != 0)
    {
        printf("\nERROR: %s - %s\n", strerror(errno), file_name);
        errno = 0;
    }
}
