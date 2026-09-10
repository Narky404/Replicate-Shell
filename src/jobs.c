
#include "jobs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

job_t jobs[MAX_JOBS];
int indice_job;
int compteur_job;

// init un job vide
void init_job(job_t *job) {
    job->id = 0;
    job->pgid = -1;
    job->cmdline = NULL;
    job->status = NULL;
}

// init tableau 
void init_tab_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        init_job(&jobs[i]);
    }
    indice_job = 0;
    compteur_job = 0;
}

// ajouter un job au tableau (pid param = PGID) 
void ajout_job(pid_t pid, char *cmd_line){
    int indice_vide = trouver_case_vide();
    if (indice_vide != -1) {
        if (!compteur_job){
            indice_job =1;
        } else {
            indice_job ++;
        }
        compteur_job++;
        jobs[indice_vide].id = indice_job; // Les IDs commencent à 1
        jobs[indice_vide].pgid = pid; // store PGID 
        jobs[indice_vide].cmdline = strdup(cmd_line);  // copier la chaîne 
        jobs[indice_vide].status = "Running"; // Par défaut, le job est en cours d'exécution 
    } else {
        printf("Erreur : Le tableau de jobs est plein.\n");
    }
}

int trouver_case_vide() { // attention le tableau se remplit toujours vers la droit et ne donne jamais une case qui a déja été occupéé !
    if (compteur_job == 0){
        indice_job = 0;
    }
    for (int i = indice_job; i < MAX_JOBS; i++) {
        if (jobs[i].id == 0) {
            return i;
        }
    }
    return -1; // Aucun job vide trouvé i
}

//supprimer un job du tableau 
void supprimer_job(char *cmd) {
    for (int i = 0; i < MAX_JOBS; i++){
        if (jobs[i].cmdline != NULL && strcmp(jobs[i].cmdline, cmd) == 0) {
            jobs[i].id = 0;
            jobs[i].pgid = -1;
            free(jobs[i].cmdline);
            jobs[i].cmdline = NULL;
            jobs[i].status = NULL;
            compteur_job--;
            break; // Sortir de la boucle après avoir supprimé le job 
        }
    }
}

void supprimer_job_by_pid(pid_t pid){
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pgid == pid) {
            jobs[i].id = 0;
            jobs[i].pgid = -1;
            if (jobs[i].cmdline) free(jobs[i].cmdline);
            jobs[i].cmdline = NULL;
            jobs[i].status = NULL;
            compteur_job--;
            break;
        }
    }
}

// le job est-il au premier plan ? (on compare avec la PGID) 
int estPremierPlan(pid_t pid){
    for (int i = 0; i < MAX_JOBS; i++){
        if (jobs[i].pgid == pid && jobs[i].status != NULL && strcmp(jobs[i].status, "Running") == 0) {
            return 1; // Le job est en premier plan 
        }
    }
    return 0; // Le job n'est pas en premier plan 
}

int estJobs(pid_t pid){
    for (int i = 0; i < MAX_JOBS; i++){
        if (jobs[i].pgid == pid) {
            return 1; // Le job existe 
        }
    }
    return 0; // Le job n'existe pas 
}

void afficher_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].id != 0) { // Afficher uniquement les jobs actifs 
            char* cmd = jobs[i].cmdline ? jobs[i].cmdline : "";
            printf("[%d] %d %s\t%s\n", jobs[i].id, jobs[i].pgid, jobs[i].status ? jobs[i].status : "", cmd);
        }
    }
}

void cree_job(pid_t pid) {
    printf("[%d] %d\n", indice_job, pid);
}

void modif_status_job(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++){
        if (jobs[i].pgid == pid) {
            if(jobs[i].status != NULL && strcmp(jobs[i].status, "Running") == 0) {
                jobs[i].status = "Stopped";
            } else if (jobs[i].status != NULL && strcmp(jobs[i].status, "Stopped") == 0) {
                jobs[i].status = "Running";
            }
            break; // Sortir de la boucle après avoir modifié le job 
        }
    }
}

pid_t getPID_indice(int indice){
    if (indice <= MAX_JOBS && indice > 0){
        return jobs[indice-1].pgid;
    } else {
        return -1;
    }
}

pid_t getPID_by_name(const char *name) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].id != 0 && jobs[i].cmdline) {
            if (strstr(jobs[i].cmdline, name) != NULL) {
                return jobs[i].pgid; /* retourne la PGID */
            }
        }
    }
    return -1;
}

void affiche_jobs_stopped(pid_t pid){
    for(int i= 0; i < MAX_JOBS; i++){
        if(jobs[i].pgid == pid){
            printf("[%d]+  %s                 ./%s", jobs[i].id, jobs[i].status, jobs[i].cmdline);
        }
    }
}