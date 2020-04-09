/*
 * find.c
 *
 *\program name: find
 *  Created on: Mar 19, 2020 00:15
 *      Author: Ibrahim Alnaif
 *
 *
 *\description: this program is used to search for items inside a directory
 *\usage: ./myfind <file or directory> [ <action> ] ...
 *	Available options are:
 *		 -type   [bcdpfls]...............Search for specific Formats
 *		 -path   [path to search i.......Search in a specific path
 *		 -name   [File Name].............Search for a specific file
 *		 -user   [Name or UID]...........Search for a specific user or user-id
 *		 -group  [group or UID]...........Search for a specific group or group-id
 *		 -nouser ........................Search for files, that belongs to no user
 *		 -nogroup........................Search for files, that belongs to no user
 *		 -print  ........................Print the result (Activated by default)
 *		 -ls     ........................gives all file information
 *   if no directory is supplied, the current directory will be used as a default
 *

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <error.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>



#define MAXLEN 256
#define NULLCHAR 1

static void print_permission_string(struct stat *buf);
static void no_argv(int argc, char ** parms);
static void do_entry(const char * entry_name, char ** parms);
static void do_dir(const char * dir_name, char ** parms);
static int do_name(const char * entry_name, char *parms);
static int do_type(const char *parms, const struct stat *entry_data);
static int do_path(const char * entry_name, char *parms);
static void do_print(const char *file_name,const int print_that);
static void do_ls(const char *path,struct stat *buf,const int print_that);
static int do_user(struct stat entry_data, const char * parms);
static int do_username(struct stat entry_data, const char * parms);
static int do_userid(struct stat entry_data, const char * parms);
static int do_nouser(struct stat entry_data);
static int do_group(struct stat entry_data, const char * parms);
static int do_groupname(struct stat entry_data, const char * parms);
static int do_groupid(struct stat entry_data, const char * parms);
static int do_nogroup(struct stat entry_data);

/**
 * \brief This funktion is the main entry point for our program execution.
 *
 *
 * This function calls the necessary functions for myfind to work properly.
 *
 * \param argc - number of arguments the program is started with
 * \param argv -  points to each argument passed to the program
 * \return 0 - if the program was successful
 */

int main (int argc, char* argv[])
{
    no_argv(argc,argv);
    do_entry(argv[1],argv);
    return 0;
}

/**
 *
 *\brief: no_argv checks the arguments supplied in the program call and sets the current directory
 *as a default if not supplied by the user
 *
 * \param argc is the count of the arguments supplied
 * \param parms is the arry of the supplied arguments (argv)
 *
 * \return: void function no value returned
 *
 */

static void no_argv(int argc, char ** parms){
    if (argc == 1) {		//if no arguments are supplied the current directory will be the default
        printf ("Default directory\n");
        parms[1]=".";
        parms[2]=NULL;} //set a NULL as last item in argv to avoid overflow

    else if(argc >= 2 ){	//if no directory is supplied the current directory will be the default
        if (*parms[1] == '-'){
            char *temp;
            int n= argc+2;
            printf ("Default directory\n");
            for (int i= n-2;i>=1; i--){  //move all supplied arguments to the right to make room for the default directory
                temp = parms[i];
                parms[i+1]=temp;
            }

            parms[1]=".";
            parms[n]=NULL;}}


}


/**
 *
 *\brief: do_dir function is responsible of opening directories
 * it's parameters are passed from the function do_entry :
 * after a successful directory open it passes each directory item to the function do_entry
 *
 *
 *
 *
 * \param dir_name is the directory name and parms are the arguments by program call (argv)
 *\return: void function no return value
 *
 */

void do_dir(const char * dir_name, char ** parms) {
    struct stat st; //a struct that is defined to store information about the item
    const struct dirent *dirent; //a structure type used to return information about directory entries
    char wholepath[sizeof(dir_name)+sizeof(dirent->d_name)+1]; //set the size of whole path + the null
    errno=0;
    DIR *dirp;
    dirp = opendir(dir_name);
    if (dirp == NULL){
		//if(errno == EACCES){
          //  /*if*/ closedir(dirp); /* == -1) {
			//	error(0,errno, "closedir"); //fatal error
			//} */
		return;
        error(0,errno, "Error while opening the Directory");
    }
	
	if(errno == EACCES){
            if (closedir(dirp) == -1) {
				error(0,errno, "closedir"); //fatal error
				} 
	return;
	}
	
	
    if (errno != 0){
        error(0,errno, "Fehler!: %s\n",dir_name);
        if (closedir(dirp) == -1) {
            error(1,errno, "closedir"); //fatal error

        }
        return;

    }
    else {
        errno = 0;
        while ((dirent = readdir(dirp)) != NULL) {
            if (strcmp(dirent->d_name, ".") != 0 && (strcmp(dirent->d_name, "..") != 0)) { //ignore if the directory is "." or ".."
                //sets wholepath size to the size of the directory and the next item to display the whole path if needed
                snprintf(wholepath, (sizeof(dir_name)+sizeof(dirent->d_name) +NULLCHAR), "%s/%s", dir_name, dirent->d_name);
                if (lstat(wholepath, &st) == -1) {
                    error(0,errno, "stat - no such file or directory");
                    exit(1);
                }
                do_entry(wholepath, parms);		//send the item to do_entry for checking
            }
            if (errno!=0){
                error(0,errno, "Fault while readdir");
            }
        }
        errno=0;
        if (closedir(dirp) == -1) {
            error(0,errno, "closedir");
            exit(1);
        }
    }
}


/*
 *
 * \brief: do_entry function takes the item name and the arguments supplied in the program call
 * it has the struct stat which can give different information about the item
 * next it checks the arguments and see which to option is active and print it if it matches the demand
 *
 *
 * \param entry_name passes the item that should be checked
 * \param parms are the arguments supplied in the program call
 *
 * \return: void function no value returned
 * */

void do_entry(const char * entry_name, char ** parms){
    struct stat entry_data; //a struct that is defined to store information about the item
    errno=0;
    int i=0; //parms counter
    int default_print = 1; //if -print is not listed in the program call then this enables default printing
    int print_this=1;		//to determine if the called function should be printed
    char buffer[MAXLEN]; //an array used as a buffer between possible_entry array and the argument
    if (lstat(entry_name, &entry_data) == -1){
        error(0,errno,"lstat failed");
        return;
    }
    const char possible_entry[10][MAXLEN] = {"-nogroup","-group", "-nouser", "-user", "-name", "-type", "-path", "-print", "-ls"};

    while (parms[++i] != NULL){
        if (*parms[i] == '-'){
            strcpy(buffer, parms[i]);
            for (int j = 0; j < 10; j++) {
                if ((strcmp(possible_entry[j], buffer)) == 0) {
                    if((strcmp(possible_entry[7], buffer)) == 0){ //turns default printing off in case -print is already in the program call
                        default_print = 0;
                    }
                    switch (j){
                        case 0:
                            print_this=do_nogroup(entry_data);
                            break;
                        case 1:
                            print_this=do_group(entry_data,parms[i+1]);
                            break;
                        case 2:
                            print_this=do_nouser(entry_data);
                            break;
                        case 3:
                            print_this=do_user(entry_data,parms[i+1]);
                            break;
                        case 4:
                            print_this=do_name(entry_name,parms[i + 1]);
                            break;
                        case 5:
                            print_this=do_type(parms[i+1],&entry_data);
                            break;
                        case 6:
                            print_this=do_path(entry_name,parms[i + 1]);
                            break;
                        case 7:
                            do_print(entry_name,print_this);
                            break;
                        case 8:
                            default_print = 0;
                            do_ls(entry_name,&entry_data,print_this);
                            break;
                        default:
                            error(0,errno, "switch-case-default");
                            exit(1);
                            break;

                    }
                }
            }

        }
    }

    if (default_print == 1){		//default print
        do_print(entry_name,print_this);

    }
    if (S_ISDIR(entry_data.st_mode)){		//if the item is a directory open it
        do_dir(entry_name,parms);
    }
}

/**
 *
 * \brief checks if the filename matches with the name given in the option
 *
 *  The function do_name searches for the given file or directory
 *  by searching for a patern with fnmatch().
 *
 * \param entry_name - file/dircectory name that is passed
 * \param parms - pattern to which the filename has to match
 *
 * \return 1 - if it's successful otherwise returns 0
 *
 */

static int do_name(const char *entry_name, char *parms) {
    const char *buff = NULL;
    errno=0;
    int control=-1;
    if(strrchr(entry_name, '/') != NULL){
        buff = (strrchr(entry_name, '/') + 1);

        if ((control=(fnmatch(parms, buff, FNM_NOESCAPE))) == 0){
            return 1;
        }
        else if (control==FNM_NOMATCH){
            return 0;
        }

        else
        {
            error(1, errno, "FNMATCH %s %s\n", parms,buff);
            return 0;
        }
        
    }
    else{
        if ((control=(fnmatch(parms, entry_name,FNM_NOESCAPE))) == 0){
            return 1;
        }
        else if (control==FNM_NOMATCH){
            return 0;
        }
        else{
            error(1, errno, "FNMATCH %s %s\n", parms,buff);
            return 0;
        }
    }
}
/**
 *
 * \brief:do_type is a function that checks for a specific type of the searched item if requested.
 *
 * \param parms is the type parameter [bcdpfls]
 * \param entry_data is the struct that contains item information
 *
 * \return 0 in case of no match or 1 if match
 *
 */


static int do_type(const char *parms, const struct stat *entry_data) {
    char type_scan = parms[0]; //copies the type parameter to check it
    switch (type_scan) {
        case 'd':// is Directory
            if ((entry_data->st_mode & S_IFMT) == S_IFDIR) {
                return 1;
            }
            break;
        case 'c':// is char special file
            if ((entry_data->st_mode & S_IFMT) == S_IFCHR) {
                return 1;
            }
            break;
        case 'b': // is block special file
            if ((entry_data->st_mode & S_IFMT) == S_IFBLK) {
                return 1;
            }
            break;
        case 'p':// is  FIFO(named pipe)
            if ((entry_data->st_mode & S_IFMT) == S_IFIFO) {
                return 1;
            }
            break;
        case 'l'://is symbolic link
            if ((entry_data->st_mode & S_IFMT) == S_IFLNK) {
                return 1;
            }
            break;
        case 's':// is socket
            if ((entry_data->st_mode & S_IFMT) == S_IFSOCK) {
                return 1;
            }
            break;
        case 'f':// is „normal“ file
            if ((entry_data->st_mode & S_IFMT) == S_IFREG) {
                return 1;
            }
            break;
        default:
            error(EXIT_FAILURE, errno, "Unknown type: %s\n", parms);
    }

    return 0;

}

/**
 *
 * \brief:print function takes the file name and prints it if print_that is active
 *
 *
 * \param: file_name is the item to print
 * \param: print_that is to check if the file should be printed or not
 *
 *
 * \return: no return value. only printing
 *
 *
 */


static void do_print(const char *file_name,const int print_that) {

    errno = 0;		//reset errno
    if (print_that==1){
        int print_control = printf("%s\n", file_name);
        if (print_control < 0) {		//in case of printing goes wrong print control will be -1
            error(0, errno, "\nError while printing\n");
        }
    }
}


/**
 *
 * \brief: do_path function searches for items inside the directories of a specific path
 *
 * \param entry_name is the item name
 * \param parms is the argument that contains the path
 *
 * \return 1 in case of success, 0 otherwise
 *
 */

static int do_path(const char *entry_name, char *parms) {
    const char *buff = NULL;
    errno=0;
    int control=-1;
    if(strrchr(entry_name, '/') != NULL){
        buff = (strrchr(entry_name, '/') + 1);

        if ((control=(fnmatch(parms, buff, FNM_NOESCAPE))) == 0){
            return 1;
        }
        else if (control==FNM_NOMATCH){
            return 0;
        }

        else
        {
            error(1, errno, "FNMATCH %s %s\n", parms,buff);
            return 0;
        }
        
    }
    else{
        if ((control=(fnmatch(parms, entry_name,FNM_NOESCAPE))) == 0){
            return 1;
        }
        else if (control==FNM_NOMATCH){
            return 0;
        }
        else{
            error(1, errno, "FNMATCH %s %s\n", parms,buff);
            return 0;
        }
    }
}

/**
 *
 * \brief  Extended output of a file.
 *
 *  Indicates the number of the inode, number of blocks, permissions, number of links,
 * Owner, Group, Last Modification Time and the name of the directory entry.
 *
 * \param path - file/dircectory name that is passed
 * \param sta buf - contains information about the file and/or directory
 * \param print_that - prints ls if value is not 0
 * \return no return value
 *
 */

static void do_ls(const char *path, struct stat *buf,const int print_that) {
    int check=0;
	
    struct passwd *pwd;
    struct group *grp;
    if(print_that==1){
        pwd = getpwuid(buf->st_uid);
        grp = getgrgid(buf->st_gid);	
		
		
        struct tm * time=NULL;
        time = localtime (&(buf->st_mtime));
        char buff [15];
        strftime(buff, sizeof(buff), "%b %e %H:%M", time);
        check=printf("  %5lu  %5lu ",buf->st_ino,buf->st_blocks/2); ////BLOCKSIZE*st_blocks / st_size ->umsetzen nur wie?
        print_permission_string(buf);
        check=printf(" %3ld %2s %8s %12ld %s %s",
                     buf->st_nlink,
                     pwd->pw_name,
                     grp->gr_name,
                     buf->st_size,
                     buff,
                     path);
				


//Leider hat diese Implementierung sehr viele Testcases am Script zum Status "failed" gezwungen -> Deshalb entfernt			
		/*char *Link = NULL;			
		if(S_ISLNK(buf->st_mode)) {
			
			Link=malloc(buf->st_size+1);
			if(Link==NULL){
				error(1, errno, "\nError while printing ls\n");
			}
			
			if(readlink(path, Link, buf->st_size) == -1) {
					error(1, errno, "\nError while printing ls\n");
			}
			else	
				{
				Link[buf->st_size]= '\0';
				check=printf(" -> %s", Link);
				//error(0, errno, "\nError while printing ls\n");
				}
			}
			check=printf("\n");
			free(Link);
			*/		

//Printf-Check für ganzes ls					
			if (check < 0) {
            error(1, errno, "\nError while printing ls\n");
			}	

    }
}

/**
 * \brief converts the entry attributes to readable permissions
 *
 * Prints i-nodes,premissions, users,groups, size, dates, paths, number of blockes etc.
 *
 *
 * \param buf - contains information about the file and/or directory
 * \returns no return value
 */

static void print_permission_string(struct stat *buf) {
    int check=0;
    struct stat fileStat = *buf;
    char type;
    int number;
    number=fileStat.st_mode& S_IFMT;
    switch(number)
    {
        case S_IFSOCK: type = 's'; break;
        case S_IFLNK: type = 'l'; break;
        case S_IFREG: type = '-'; break;
        case S_IFBLK: type = 'b'; break;
        case S_IFDIR: type = 'd'; break;
        case S_IFCHR: type = 'c'; break;
        case S_IFIFO: type= 'p'; break;
        default: type = '-';
    }

    check=printf( "%c",type);
    check=printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    check=printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
//	check=printf( (fileStat.st_mode & S_IXUSR& 04000) ? "s" : "x");
//	check=printf( (!fileStat.st_mode & S_IXUSR& 04000) ? "S" : "-");
    check=printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    check=printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    check=printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    check=printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    check=printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    check=printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    check=printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
//	check=printf( (fileStat.st_mode & S_IWOTH & 01000==01000) ? "t" : "-");
//	check=printf( (fileStat.st_mode & S_IXOTH) ? "T" : "-");
    if (check < 0) {
        error(0, errno, "Error argv check while printing");
    }
}

/**
*
* \brief checks the file is owned by the user
*
* The function do_user checks if the file is owned by a user based on
 * the user name or on the numeric user id.
*
*
* \param data_entry - contains information about the file and/or directory
* \param parms - the username or id to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/
static int do_user(struct stat data_entry, const char * parms) {
    int match;
    if(do_username(data_entry, parms) || do_userid(data_entry, parms) == 1){
        match = 1;
    }
    else {
        match = 0;
    }
    return match;
}


/**
*
* \brief checks if items are owned by someone with a specific user name
*
* This function checks if items belong to someone with a specific user name or not
*
*
* \param entry_data - contains information about the file and/or directory
* \param parms - the username to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/


static int do_username(struct stat entry_data, const char * parms) {
    int match;
    struct passwd *pwd_entry;
    errno = 0;
    pwd_entry = getpwnam(parms);
    if (pwd_entry != NULL)
    {
        if((entry_data.st_uid == pwd_entry->pw_uid)==1){
            match = 1;
        }
        else{
            match = 0;
        }
    }
    else
    {
        match = 0;
        if (errno != 0)
            error(0, errno, "\nError - user\n");
    }
    return match;
}

/**
*
* \brief checks if items are owned by someone with a specific user ID
*
* This function checks if items belong to someone with a specific user ID or not
*
*
* \param entry_data - contains information about the file and/or directory
* \param parms - the user ID to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/

static int do_userid(struct stat entry_data, const char * parms) {
    int match;
    uid_t uid;
    uid = (uid_t)strtol(parms,NULL,0);
    if((entry_data.st_uid == uid)== 1 ){
        match = 1;
    }else{
        match = 0;
    }
    return match;
}

/**
*
* \brief checks if items don't belong to a user
*
* This function checks if items don't belong to a user and outpusts those
*
*
* \param entry_data - contains information about the file and/or directory
*
* \return 1 - if it's successful otherwise returns 0
*
*/
static int do_nouser(struct stat entry_data){
    int match;
    struct passwd *pwd_entry;

    if ((pwd_entry = getpwuid(entry_data.st_uid)) != NULL) {
        if (errno!=0) {
            error(0, errno, "\nError - nogroup\n");
        }
        match = 0;
    }
    match = 1;
    return match;
}

/**
*
* \brief checks if items are owned by the group entered
*
* The function do_group checks if the item is owned by a group based on
 * the groupname or on the numeric group ID.
*
*
* \param data_entry - contains information about the file and/or directory
* \param parms - group name or id to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/
static int do_group(struct stat entry_data, const char * parms){
    int result;
    if(do_groupname(entry_data, parms) || do_groupid(entry_data, parms)== 1 ){
        result = 1;
    }
    else {

        result = 0;
    }
    return result;
}

/**
*
* \brief checks if items are owned by someone with a specific group name
*
* This function checks if items belong to someone with a specific group name or not
*
*
* \param entry_data - contains information about the file and/or directory
* \param parms - group name to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/
static int do_groupname(struct stat entry_data, const char * parms)
{
    int match;
    struct group *gr_entry;
    errno = 0;
    gr_entry = getgrnam(parms);
    if (gr_entry != NULL)
    {
        match = (entry_data.st_gid == gr_entry->gr_gid);
    }
    else
    {
        match = 0;
        if (errno != 0) {
            error(0, errno, "\nError - group\n");
        }
    }
    return match;
}

/**
*
* \brief checks if items are owned by someone with a specific group ID
*
* This function checks if items belong to someone with a specific group ID or not
*
*
* \param entry_data - contains information about the file and/or directory
* \param parms - group ID to be checked
*
*
* \return 1 - if it's successful otherwise returns 0
*
*/

static int do_groupid(struct stat entry_data, const char * parms)
{
    int match;
    gid_t gid;
    gid = (gid_t)strtol(parms,NULL,0);
    if((entry_data.st_gid == gid) ==  1){
        match = 1;
    }else {
        match = 0;
    }
    return match;
}

/**
*
* \brief checks if items don't belong to a group
*
* This function checks if items don't belong to a group and outpusts those
*
*
* \param entry_data - contains information about the file and/or directory
*
* \return 1 - if it's successful otherwise returns 0
*
*/
static int do_nogroup(struct stat entry_data){
    int match;
    struct group *gr_entry;
    if ((gr_entry = getgrgid(entry_data.st_gid)) != NULL) {
        if (errno!=0) {
            error(0, errno, "\nError - nogroup\n");
        }
        match = 0;
    }

    match = 1;
    return match;
}
