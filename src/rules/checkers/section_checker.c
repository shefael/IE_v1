/*
 * section_checker.c — Vérificateurs de structure de document
 *
 * Stratégie de détection des sections :
 * On cherche une ligne qui contient exactement le titre de section
 * (insensible à la casse, avec trim des espaces).
 * Une "ligne de titre" est une ligne dont le contenu correspond
 * au paramètre de la règle.
 */

#include "rules/checkers/section_checker.h"
#include "rules/rule_engine.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* --------------------------------------------------------------------------
 * Utilitaires internes
 * -------------------------------------------------------------------------- */

/*
 * Retourne une copie de la chaîne en minuscules ASCII.
 * Le appelant doit libérer le résultat.
 */
static char *minuscules(const char *src)
{
    if (!src) return NULL;
    size_t len = strlen(src);
    char  *dst = (char *)malloc(len + 1);
    if (!dst) return NULL;
    for (size_t i = 0; i <= len; i++) {
        dst[i] = (char)tolower((unsigned char)src[i]);
    }
    return dst;
}

/*
 * Trim des espaces en début et fin d'une chaîne (modifie en place).
 */
static void trim(char *s)
{
    if (!s) return;
    /* Trim gauche */
    size_t debut = 0;
    while (s[debut] && isspace((unsigned char)s[debut])) debut++;
    if (debut > 0) memmove(s, s + debut, strlen(s) - debut + 1);
    /* Trim droite */
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

/*
 * Cherche la position (en octets) d'une section dans le texte.
 * Retourne -1 si non trouvée.
 * La recherche est insensible à la casse.
 * Une section est détectée si une ligne contient le titre cherché.
 */
static long trouver_section(const char *texte, const char *titre)
{
    if (!texte || !titre) return -1;

    char *texte_min  = minuscules(texte);
    char *titre_min  = minuscules(titre);
    if (!texte_min || !titre_min) {
        free(texte_min);
        free(titre_min);
        return -1;
    }

    /* Trim du titre recherché */
    trim(titre_min);
    size_t len_titre = strlen(titre_min);

    long   position  = -1;
    size_t pos       = 0;
    size_t len_texte = strlen(texte_min);

    while (pos < len_texte) {
        /* Trouver le début et la fin de la ligne courante */
        size_t debut_ligne = pos;
        size_t fin_ligne   = pos;
        while (fin_ligne < len_texte &&
               texte_min[fin_ligne] != '\n') {
            fin_ligne++;
        }

        /* Extraire la ligne et la trimmer */
        size_t len_ligne = fin_ligne - debut_ligne;
        if (len_ligne < 512) {
            char ligne[512];
            memcpy(ligne, texte_min + debut_ligne, len_ligne);
            ligne[len_ligne] = '\0';
            trim(ligne);

            /*
             * Vérification : la ligne contient-elle le titre ?
             * On cherche le titre comme sous-chaîne de la ligne.
             */
            if (strlen(ligne) > 0 && strstr(ligne, titre_min) != NULL) {
                /* Vérification supplémentaire : le titre représente
                 * au moins 50% de la ligne (évite les faux positifs) */
                if (len_titre * 2 >= strlen(ligne)) {
                    position = (long)debut_ligne;
                    break;
                }
            }
        }

        pos = fin_ligne + 1;
    }

    free(texte_min);
    free(titre_min);
    return position;
}

/*
 * Construit un RuleResult avec les champs remplis.
 */
static RuleResult faire_resultat(const char *rule_id,
                                  RuleStatus statut,
                                  const char *message)
{
    RuleResult res;
    // Remplacer l'assignation par une copie sécurisée
if (rule_id) {
    strncpy(res.rule_id, rule_id, sizeof(res.rule_id) - 1);
    res.rule_id[sizeof(res.rule_id) - 1] = '\0';
} else {
    strncpy(res.rule_id, "?", sizeof(res.rule_id) - 1);
}
    res.statut = statut;
    if (message) {
    strncpy(res.message, message, sizeof(res.message) - 1);
    res.message[sizeof(res.message) - 1] = '\0'; // Assure la terminaison nulle
} else {
    res.message[0] = '\0'; // Message vide
}
    return res;
}

/* --------------------------------------------------------------------------
 * check_section_exists
 * -------------------------------------------------------------------------- */
RuleResult check_section_exists(const Rule *rule, const char *texte)
{
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION,
                              "Paramètre invalide");
    }

    /* Le paramètre est le titre de section à chercher */
    const char *titre = rule->param_chaine;
    if (!titre || titre[0] == '\0') {
        return faire_resultat(id, STATUS_WARNING,
                              "Paramètre manquant dans la règle");
    }

    long pos = trouver_section(texte, titre);

    if (pos >= 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Section \"%s\" présente", titre);
        return faire_resultat(id, STATUS_OK, msg);
    } else {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Section \"%s\" absente", titre);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }
}

/* --------------------------------------------------------------------------
 * check_section_order
 * -------------------------------------------------------------------------- */
RuleResult check_section_order(const Rule *rule, const char *texte)
{
    const char *id = rule ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION,
                              "Paramètre invalide");
    }

    /*
     * Le paramètre est un ParamSectionOrder* contenant un tableau
     * de chaînes représentant l'ordre attendu des sections.
     */
    const ParamSectionOrder *param = &rule->param_so;
    if (!param || param->nb_sections == 0) {
        return faire_resultat(id, STATUS_WARNING,
                              "Aucune section définie dans la règle");
    }

    /* Trouver la position de chaque section dans le texte */
    long position_precedente = -1;
    int  sections_manquantes = 0;
    int  ordre_incorrect     = 0;
    char msg[512]            = {0};

    for (int i = 0; i < param->nb_sections; i++) {
        const char *titre = param->sections[i];
        if (!titre) continue;

        long pos = trouver_section(texte, titre);

        if (pos < 0) {
            /* Section absente : on continue sans bloquer l'ordre */
            sections_manquantes++;
            continue;
        }

        if (pos <= position_precedente) {
            /* Section trouvée mais dans le mauvais ordre */
            ordre_incorrect++;
            snprintf(msg, sizeof(msg),
                     "Section \"%s\" mal positionnée", titre);
        }

        position_precedente = pos;
    }

    if (ordre_incorrect > 0) {
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    if (sections_manquantes > 0) {
        snprintf(msg, sizeof(msg),
                 "%d section(s) absente(s) mais ordre respecté",
                 sections_manquantes);
        return faire_resultat(id, STATUS_WARNING, msg);
    }

    return faire_resultat(id, STATUS_OK,
                          "Ordre des sections respecté");
}

/* --------------------------------------------------------------------------
 * check_heading_format
 * -------------------------------------------------------------------------- */
RuleResult check_heading_format(const Rule *rule, const char *texte)
{
    const char *id = (rule && rule->id[0] != '\0') ? rule->id : "?";

    if (!rule || !texte) {
        return faire_resultat(id, STATUS_VIOLATION,
                              "Paramètre invalide");
    }

    // On prend l'adresse du membre param_hf qui est déjà du bon type
    const ParamHeadingFormat *param = &rule->param_hf;
    if (!param) {
        return faire_resultat(id, STATUS_WARNING,
                              "Paramètre manquant dans la règle");
    }

    /*
     * Stratégie : on cherche les lignes qui ressemblent à des titres
     * (ligne courte, non vide, entourée de lignes vides ou en début).
     * On vérifie ensuite que le format correspond au paramètre.
     *
     * Pour "uppercase" : tous les caractères alphabétiques ASCII
     * doivent être en majuscules.
     */
    int  nb_titres      = 0;
    int  nb_incorrects  = 0;
    char dernier_incorrect[256] = {0};

    const unsigned char *p   = (const unsigned char *)texte;
    size_t               pos = 0;
    size_t               len = strlen(texte);

    while (pos < len) {
        size_t debut_ligne = pos;
        size_t fin_ligne   = pos;

        while (fin_ligne < len && p[fin_ligne] != '\n') fin_ligne++;

        size_t len_ligne = fin_ligne - debut_ligne;

        /* On considère un titre : ligne de 3 à 80 caractères non vide */
        if (len_ligne >= 3 && len_ligne <= 80) {
            char ligne[81];
            memcpy(ligne, texte + debut_ligne, len_ligne);
            ligne[len_ligne] = '\0';
            trim(ligne);

            size_t len_trim = strlen(ligne);
            if (len_trim >= 3 && len_trim <= 60) {
                nb_titres++;

                /* Vérifier le format */
                if (strcmp(param->format, "uppercase") == 0) {
                    int est_uppercase = 1;
                    for (size_t i = 0; i < len_trim; i++) {
                        unsigned char c = (unsigned char)ligne[i];
                        /* Vérifier uniquement les lettres ASCII */
                        if (c >= 'a' && c <= 'z') {
                            est_uppercase = 0;
                            break;
                        }
                    }
                    if (!est_uppercase) {
                        nb_incorrects++;
                        strncpy(dernier_incorrect, ligne, 255);
                    }
                } else if (strcmp(param->format, "lowercase") == 0) {
                    int est_lowercase = 1;
                    for (size_t i = 0; i < len_trim; i++) {
                        unsigned char c = (unsigned char)ligne[i];
                        if (c >= 'A' && c <= 'Z') {
                            est_lowercase = 0;
                            break;
                        }
                    }
                    if (!est_lowercase) {
                        nb_incorrects++;
                        strncpy(dernier_incorrect, ligne, 255);
                    }
                }
            }
        }

        pos = fin_ligne + 1;
    }

    if (nb_titres == 0) {
        return faire_resultat(id, STATUS_WARNING,
                              "Aucun titre détecté dans le document");
    }

    if (nb_incorrects > 0) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "%d titre(s) non conforme(s). Ex: \"%s\"",
                 nb_incorrects, dernier_incorrect);
        return faire_resultat(id, STATUS_VIOLATION, msg);
    }

    return faire_resultat(id, STATUS_OK,
                          "Format des titres conforme");
}