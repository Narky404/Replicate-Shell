#include "execcmd.h"

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

char* cmd_precedent;
pid_t fg_pids[10];  // PIDs des processus au premier plan
int num_fg_procs = 0;  // Nombre de processus au premier plan

void exec_cmd_int(struct cmdline *l) {
    // Commande pour quitter
    if (l->seq[0] != NULL && (strcmp(l->seq[0][0], "quit") == 0 || strcmp(l->seq[0][0], "exit") == 0)) {
        printf("exit\n");
        exit(0);
    }
    
    // Commande cd
    if (l->seq[0] != NULL && strcmp(l->seq[0][0], "cd") == 0) {
        if(l->seq[0][1] != NULL){ // on lui donne un repertoire vers où aller
            if ( chdir(l->seq[0][1]) != 0){
                fprintf(stderr, "bash: cd: %s: %s\n", l->seq[0][1], strerror(errno)); // on n'utilise pas unix_error pour ne pas quitter le shell
            }
        } else { // si cd seule alors on doit aller au home
            const char * home = getenv( "HOME" ); // récuperation du chemin pour avoir l'home du l'utilisateur
            if(home == NULL){
                fprintf(stderr, "Chemin vers le Home n'existe pas.\n");
                exit(-1);
            }
            chdir(home);
        }
    }

    // Commande jobs
    if(l->seq[0] != NULL && strcmp(l->seq[0][0], "jobs") == 0) {
        afficher_jobs();
    }

    // TODO fg et bg
    if(l->seq[0] != NULL && strcmp(l->seq[0][0], "fg") == 0) {
        char *arg = NULL;
        if (l->seq[0][1] != NULL) arg = l->seq[0][1];
        if (arg == NULL) {
            fprintf(stderr, "fg: usage: fg %%job or fg name\n");
        } else {
            pid_t pgid = -1;
            if (arg[0] == '%') {
                //On cherche le job par indice
                if (arg[1] && isdigit((unsigned char)arg[1])) {
                    int indice = atoi(&arg[1]);
                    pgid = getPID_indice(indice);
                } else if (arg[1]) { //On cherche le job par nom
                    pgid = getPID_by_name(&arg[1]);
                }
            } else {
                pgid = getPID_by_name(arg);
            }

            if (pgid > 0) {
                // Continuer le groupe
                
                num_fg_procs = 1;
                fg_pids[0] = pgid;
                kill(-pgid, SIGCONT);
                
                int status;
                pid_t pid_;
                while ( (pid_ = waitpid(-pgid, &status, WUNTRACED) ) > 0);

                num_fg_procs = 0;
            } else {
                fprintf(stderr, "fg: %s: no such job\n", arg);
            }
        }
    }

    cmd_precedent = strdup(l->seq[0][0]);
}

void exec_cmd(struct cmdline *l, int num_cmds) {

    cmd_precedent = strdup(l->seq[0][0]);
    
    pid_t pids[num_cmds]; // Tableau pour stocker les PID des processus
    if (num_cmds == 1) {
        pids[0] = Fork();

        if (pids[0] == 0) {
            // Processus fils : gestion de la redirection
            
            //Signal(SIGTSTP, SIG_IGN );
            //Signal(SIGINT, SIG_DFL);

            // Put child in its own process group
            setpgid(0, 0);

            if (l->in) {
                int fd_in = Open(l->in, O_RDONLY, 0);
                Dup2(fd_in, STDIN_FILENO);
                Close(fd_in);
            }
            if (l->out) {
                int fd_out = Open(l->out, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                Dup2(fd_out, STDOUT_FILENO);
                Close(fd_out);
            }
            
            if (execvp(l->seq[0][0], l->seq[0]) == -1) {
                fprintf(stderr, "%s: command not found \n", l->seq[0][0]);
            }
            // TODO supp pid du tableau vu qu'il a réussi
        }
        else {
            // Parent: ensure child is in its own pgid
            setpgid(pids[0], pids[0]);
        }
    } else {
        // Pipeline : plusieurs commande
        int prev_fd = -1; // fin de lecture du tube précédent
        int pipefd[2];
        for (int i = 0; i < num_cmds; i++) {

            cmd_precedent = strdup(l->seq[i][0]);
            if (i < num_cmds - 1) {
                // Création d'un tube pour relier la commande actuel à la commande suivant
                pipe(pipefd);
            }

            pids[i] = Fork();
            // TODO jobs 
            
            if (pids[i] == 0) {

                Signal(SIGTSTP, SIG_IGN);  // Ignorer SIGTSTP dans le fils
                Signal(SIGINT, SIG_DFL);

                // Processus fils
                if (i > 0) {
                    // Redirection  l'entrée standard depuis le tube précédent
                    Dup2(prev_fd, STDIN_FILENO);
                } else if (l->in) { 
                    // Pour la première command
                    int fd_in = Open(l->in, O_RDONLY, 0);
                    Dup2(fd_in, STDIN_FILENO);
                    Close(fd_in);
                }
                if (i < num_cmds - 1) {
                    // Redirection la sortie standard vers le tube courant
                    Dup2(pipefd[1], STDOUT_FILENO);
                } else if (l->out) {
                    // Pour la dernière commande
                    int fd_out = Open(l->out, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    Dup2(fd_out, STDOUT_FILENO);
                    Close(fd_out);
                }
                if (i > 0)
                    Close(prev_fd);
                if (i < num_cmds - 1) {
                    Close(pipefd[0]);
                    Close(pipefd[1]);
                }
                execvp(l->seq[i][0], l->seq[i]);
                fprintf(stderr, "%s: command not found\n", l->seq[i][0]);
                exit(1);
            } else {
                // Processus père : gérer les tubes
                // Set process group for pipeline: make all children part of first child's pgid
                if (i == 0) {
                    setpgid(pids[0], pids[0]);
                } else {
                    setpgid(pids[i], pids[0]);
                }
                if (i > 0)
                    Close(prev_fd);
                if (i < num_cmds - 1) {
                    prev_fd = pipefd[0];
                    Close(pipefd[1]);
                }
            }
        }
    }
    // Attendre les processus si pas en arrière-plan
    num_fg_procs = 0;
    if (!l->background) {
        
        // Sauvegarder la PGID pour le gestionnaire SIGTSTP
        num_fg_procs = 1;
        fg_pids[0] = pids[0]; // stocke la PGID (first child pid)
        
        // Give terminal control to the foreground job (process group of first pid)
        pid_t pgid = pids[0];
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(STDIN_FILENO, pgid);
        signal(SIGTTOU, SIG_DFL);
        
        // Wait for the job (the whole process group) to change state
        {
            int status;
            waitpid(-pgid, &status, WUNTRACED);
            // If the job was stopped, keep it in jobs table via signal handler
        }
        
        // Restore terminal to shell
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(STDIN_FILENO, getpgrp());
        signal(SIGTTOU, SIG_DFL);
        
    } else {
        // Mode arrière-plan : n'ajoute qu'un seul job par pipeline (PGID = pids[0])
        char cmdbuf[256];
        strncpy(cmdbuf, l->seq[0][0], sizeof(cmdbuf)-1);
        cmdbuf[sizeof(cmdbuf)-1] = '\0';
        // retirer chemin si présent
        char *cmd = strrchr(cmdbuf, '/');
        if (cmd) cmd++;
        else cmd = cmdbuf;
        ajout_job(pids[0], cmd);
        cree_job(pids[0]);
    }
}