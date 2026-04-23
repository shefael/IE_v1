IntelliEditor



Éditeur de texte intelligent hors ligne pour Windows, développé en C.\*\*



> Projet en phase de développement actif. Version actuelle : Phase 0 (squelette compilable).



&#x20;🎯 Objectif



IntelliEditor est un éditeur de texte léger intégrant :



\- Un moteur de règles linguistiques personnalisable (JSON),

\- Un correcteur orthographique basé sur Hunspell,

\- Un assistant LLM local via llama.cpp (modèles GGUF),

\- Une interface native Windows utilisant Scintilla.



Tout fonctionne \*\*hors ligne\*\*, sans dépendance cloud.



&#x20;Prérequis



\- \*\*Système\*\* : Windows 10/11

\- \*\*MSYS2\*\* avec \*\*MinGW-w64\*\* (GCC 13+)

\- \*\*CMake\*\* 3.20 ou supérieur

\- \*\*Git\*\*



Les bibliothèques suivantes sont incluses en tant que sous-modules Git ou sources externes :



| Bibliothèque | Rôle |

|--------------|------|

| \[Scintilla](https://scintilla.org/) | Composant d'édition de texte |

| \[cJSON](https://github.com/DaveGamble/cJSON) | Parsing JSON pour les règles |

| \[llama.cpp](https://github.com/ggerganov/llama.cpp) | Inférence LLM locale |

| Hunspell (dictionnaires) | Correction orthographique |



&#x20;Compilation



```bash

\# Cloner le dépôt

git clone https://github.com/shefael/IE\_v1.git

cd IntelliEditor



\# Initialiser les sous-modules (si utilisés)

git submodule update --init --recursive



\# Configurer avec CMake (MinGW Makefiles)

cmake -B build -G "MinGW Makefiles"



\# Compiler

cmake --build build



L'exécutable généré est build/IntelliEditor.exe.



&#x20;Structure du projet

IntelliEditor/

├── include/          # Headers du projet (gap\_buffer, ui, llm, rules...)

├── src/              # Sources C correspondantes

├── external/         # Bibliothèques tierces (cJSON, Scintilla, llama.cpp)

├── data/             # Dictionnaires Hunspell et templates de règles

├── models/           # Modèles LLM au format GGUF (non versionnés)

├── tests/            # Futurs tests unitaires

├── docs/             # Documentation (architecture, notes)

├── CMakeLists.txt    # Build system

└── README.md



État actuel (Phase 0)

Squelette de projet compilable



Intégration des dépendances externes



Stubs des modules principaux (gap buffer, UI, LLM, règles, encodage, config)



Interface graphique fonctionnelle (Phase 1)



Moteur de règles et liaison LLM (Phase 2



Auteur

Shefael – @shefael





