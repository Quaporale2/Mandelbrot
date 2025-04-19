#!/bin/bash

# Ajoute tout les fichiers pour les mettre à jours
git add .

# Envoie les modifications au git
git commit -m "MAJ"

# Pousse les modifications au rrepository Github
git push --tags

