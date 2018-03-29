#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>


static void pparameter(const char **arguments, const char ** parameters, int argument_count);

static void do_dir (const char * file_name, const char * const *parms);
static void do_file (const char * file_name, const char *const *parms);

static void errmsg(int error_number, char *file_name);

// Funktionen zum Checken der Parameter
static void check_user_name(char *current_path, const char *user, struct stat current_entry);
static void check_uid(char *current_path, int user_id, struct stat current_entry);

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
        do_file(newpath, parms);

    }


}



static void do_file (const char * file_name, const char *const *parms)
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
static void check_uid(char *current_path, int user_id, struct stat current_entry)
{
    struct passwd *popt;
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
