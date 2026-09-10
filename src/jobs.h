#include "csapp.h"

#define MAX_JOBS 10

/* Structure du jobs */
typedef struct {
    int id; // indice du job {1 ... MAXJOBS}
    pid_t pgid; // pgid > 0
    char *cmdline; // command line 
    char *status; // "Running" ou "Stopped" 
} job_t;

#ifndef __JOBS_GLOBAL_
#define __JOBS_GLOBAL_

extern job_t jobs[MAX_JOBS]; // tableau de jobs (on stocke ici la PGID dans le champ pid)
extern int indice_job; // indice du prochain job à ajouter
extern int compteur_job;

#endif

void init_job(job_t *job);

void init_tab_jobs();

void ajout_job(pid_t pid, char *cmdline);

int trouver_case_vide();

void supprimer_job(char *cmd);

int estPremierPlan(pid_t pid);

int estJobs(pid_t pid);

void afficher_jobs();

void cree_job(pid_t pid);

void modif_status_job(pid_t pid);

pid_t getPID_indice(int indice);

void supprimer_job_by_pid(pid_t pid);

pid_t getPID_by_name(const char *name);

void affiche_jobs_stopped(pid_t pid);