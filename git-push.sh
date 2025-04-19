#!/bin/bash

# Ajoute tous les fichiers
git add .

# Commit avec un message
git commit -m "MAJ"

# Pousse les commits vers la branche courante
git push

# Synchronise les tags
echo "🔄 Synchronisation des tags..."

# Récupère tous les tags distants (silencieusement)
git fetch --tags --quiet

# Liste des tags locaux et distants
tags_locaux=$(git tag)
tags_distants=$(git ls-remote --tags origin | cut -f2 | sed 's|refs/tags/||' | grep -v '\^{}')

# Recherche des tags distants absents localement
tags_a_supprimer=()
for tag in $tags_distants; do
    if ! echo "$tags_locaux" | grep -q "^$tag$"; then
        tags_a_supprimer+=("$tag")
    fi
done

# Avertissement si des tags seraient supprimés
if [ ${#tags_a_supprimer[@]} -gt 0 ]; then
    echo "⚠️  Les tags suivants existent sur le dépôt distant, mais pas en local :"
    for tag in "${tags_a_supprimer[@]}"; do
        echo "  - $tag"
    done
    echo
    echo "❓ Souhaitez-vous les supprimer du dépôt distant pour refléter l'état local ? (o/N)"
    read -r reponse

    if [[ "$reponse" == "o" || "$reponse" == "O" ]]; then
        for tag in "${tags_a_supprimer[@]}"; do
            echo "🗑️  Suppression du tag distant : $tag"
            git push origin --delete "$tag"
        done
    else
        echo "🚫 Suppression de tags distants annulée."
    fi
fi

# Envoie tous les tags locaux restants vers le dépôt
echo "🚀 Envoi des tags locaux..."
git push --tags
