#ifndef EDITOR_EXPORTER_H
#define EDITOR_EXPORTER_H

/*
 * exporter.h — Export du document vers différents formats
 *
 * Formats supportés :
 *   .txt  — Texte brut UTF-8
 *   .rtf  — Rich Text Format avec styles
 *   .ie   — Format binaire propriétaire IntelliEditor
 */

#include "editor/formatter.h"
#include <stddef.h>

/* --------------------------------------------------------------------------
 * Codes de retour
 * -------------------------------------------------------------------------- */
#define EXPORT_OK               0
#define EXPORT_ERR_FICHIER     -1   /* Impossible d'ouvrir/créer le fichier */
#define EXPORT_ERR_MEMOIRE     -2   /* Allocation échouée */
#define EXPORT_ERR_PARAM       -3   /* Paramètre invalide (NULL) */
#define EXPORT_ERR_ECRITURE    -4   /* Erreur d'écriture disque */

/* --------------------------------------------------------------------------
 * API publique
 * -------------------------------------------------------------------------- */

/*
 * Exporte le texte brut en UTF-8.
 * texte     : contenu UTF-8 du document
 * chemin    : chemin du fichier de destination (UTF-8)
 */
int exporter_txt(const char *chemin, const char *texte);

/*
 * Exporte en RTF avec les styles de la table.
 * texte     : contenu UTF-8 du document
 * ts        : table des styles (peut être NULL → pas de styles)
 * chemin    : chemin du fichier de destination (UTF-8)
 */
int exporter_rtf(const char *chemin, const char *texte,
                 const TableStyles *ts);

/*
 * Exporte au format binaire propriétaire .ie
 *
 * Structure du fichier :
 *   [4 octets] Magic number : 'I','N','T','E'
 *   [2 octets] Version      : uint16_t little-endian = 1
 *   [4 octets] Taille texte : uint32_t little-endian (octets UTF-8)
 *   [N octets] Texte brut UTF-8
 *   [4 octets] Nb entrées style : uint32_t little-endian
 *   Pour chaque entrée :
 *     [4 octets] offset   : uint32_t little-endian
 *     [4 octets] longueur : uint32_t little-endian
 *     [1 octet]  style_id : uint8_t
 */
int exporter_ie(const char *chemin, const char *texte,
                const TableStyles *ts);

/*
 * Importe un fichier .txt (UTF-8) et retourne son contenu.
 * Le appelant doit libérer le résultat avec free().
 * Retourne NULL en cas d'erreur.
 */
char *importer_txt(const char *chemin);

#endif /* EDITOR_EXPORTER_H */