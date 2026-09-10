/*
 * Copyright (C) 2002, Simon Nieuviarts
*/
#include <signal.h>
#include <setjmp.h>

#include "execcmd.h"

extern pid_t fg_pids[10];
extern int num_fg_procs;
extern char* cmd_precedent;

/* Variable globale pour gérer SIGINT */
static sigjmp_buf jmp_sigint;


/* Etape 9 Gestion des zombis */
void sigchld_handler(int sig){
    pid_t pid;
    int status;
    while ( (pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0);
}

void sigint_handler(int sig){
    printf("\n");
    pid_t pgid = fg_pids[0];
    if (estJobs(pgid)){
        supprimer_job_by_pid(pgid);
        kill(pgid, SIGINT);
    }
    siglongjmp(jmp_sigint, 1); // capte SIGINT et jump vers où sigsetjmp() a été appelé
}

void sigtstp_handler(int sig){
    // Ajouter les processus au premier plan au tableau de jobs avec le statut "Stopped"
    if(fg_pids[0] == -2){
        siglongjmp(jmp_sigint, 1);
    }

    printf("\n");

    if(fg_pids[0] != -2){
        pid_t pid_forground = fg_pids[0];
        if (!estJobs(pid_forground)) {
            // Ajouter le job (on stocke la pid_forground)
            ajout_job(pid_forground, cmd_precedent);
        }
        // Modifier le statut à "Stopped"
        modif_status_job(pid_forground);
        affiche_jobs_stopped(pid_forground);
        kill(-pid_forground, SIGTSTP);
    }
}

int main(){

    Signal(SIGTSTP, sigtstp_handler);
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler);
    
    while (1) {
        fg_pids[0] = -2;
        if (sigsetjmp(jmp_sigint, 1) != 0) { // maj de où le jump doit aller
            //!= 0 car : la valeur de retour de sigsetjmp = 0 pour appel initial, et !=0 lors du retour via siglongjmp
            continue;
        }
        
        char chemin[100];
        getcwd(chemin, sizeof(chemin));
        printf("\e[1m\033[33m%s >\033[0m\e[m ", chemin );
        fflush(stdout);
        
        struct cmdline *l;
        l = readcmd();
        
        // gestion du cas où l'utilisateur fait des retour à la ligne
        if(l->seq[0]==NULL){
            continue;
        }

        // Si le flux est fermé, fin normale
        if (!l) {
            printf("exit\n");
            exit(0);
        }

        if (l->err) {
            // Erreur de syntaxe, on lit une autre commande
            fprintf(stderr, "error: %s\n", l->err);
            continue;
        }

        exec_cmd_int(l); //on execute les commandes internes

        int num_cmds = 0;
        while (l->seq[num_cmds] != NULL)
            num_cmds++;
        if (num_cmds == 0)
            continue;
        
        // attention : on ne doit pas exécuter les commandes internes avec exec_cmd, sinon on a un message d'erreur alors que la cmd a été faite 
        if(strcmp(l->seq[0][0], "jobs") == 0 || strcmp(l->seq[0][0], "cd") == 0 || strcmp(l->seq[0][0], "fg") == 0 || strcmp(l->seq[0][0], "bg") == 0){
            continue;
        }

        exec_cmd(l, num_cmds); //on execute les commandes externes
    }
    return 0;
}
