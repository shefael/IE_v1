\# Architecture du projet IntelliEditor



\## Structure générale



IntelliEditor est un éditeur de texte intelligent hors ligne pour Windows, développé en C avec une interface basée sur Scintilla et un moteur de règles connecté à un LLM local via llama.cpp.



\## Dépendances externes



| Bibliothèque | Version / Commit | Emplacement dans le projet | Origine | Notes d'intégration |

|--------------|------------------|---------------------------|---------|---------------------|

| \*\*cJSON\*\* | Dernière (post 1.7.18) | `external/cJSON/` | https://github.com/DaveGamble/cJSON | Compilé en bibliothèque statique via `add\_library(cjson STATIC external/cJSON/cJSON.c)`. |

| \*\*Scintilla\*\* | 5.5.3+ (sources récentes) | `external/scintilla/` | https://github.com/ScintillaOrg/scintilla | Nécessite C++17. Correction appliquée : `target\_compile\_options(scintilla PRIVATE -include cstdint)` pour résoudre les problèmes de types `uint8\_t`. Compilé en bibliothèque statique avec les sources Win32. |

| \*\*llama.cpp\*\* | Commit récent (037bfe38d) | `external/llama.cpp/` | https://github.com/ggerganov/llama.cpp | Intégré via `add\_subdirectory()`. Désactivation des exemples et tests (`LLAMA\_BUILD\_EXAMPLES=OFF`, etc.). Linkage avec `llama` et `ggml`. |

| \*\*PCRE2\*\* | (Non intégré pour le moment) | `external/pcre2/` (optionnel) | https://github.com/PCRE2Project/pcre2 | Désactivé temporairement dans le CMakeLists.txt. Pourra être réactivé pour les expressions régulières avancées. |



\## Modèles LLM



Les modèles au format GGUF sont stockés dans `models/` et \*\*ne sont pas versionnés\*\* (exclus via `.gitignore`).



| Modèle | Chemin | Utilisation prévue |

|--------|--------|-------------------|

| LLaMA 2/3 | `models/llama.gguf` | Complétion générale, génération de suggestions |

| Mistral | `models/mistral.gguf` | Analyse de texte, reformulation |



\## Dictionnaires Hunspell



Les fichiers de dictionnaire pour la correction orthographique sont dans `data/hunspell/`.



| Langue | Fichiers | Statut |

|--------|----------|--------|

| Français | `fr\_FR.aff`, `fr\_FR.dic` | Présents, à valider |



\## Compilation



Le projet utilise \*\*CMake 3.20+\*\* et \*\*MinGW-w64 (GCC 15.2.0)\*\* sous Windows.



```bash

\# Configuration

cmake -B build -G "MinGW Makefiles"



\# Construction

cmake --build build

L'exécutable généré est build/IntelliEditor.exe.


