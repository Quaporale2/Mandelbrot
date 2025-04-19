#!/bin/bash

# Vérifie s'il y a des fichiers modifiés localement
if ! git diff-index --quiet HEAD --; then
    echo "⚠️  Attention : des modifications locales non enregistrées ont été détectées."
    echo "Si vous continuez, elles pourraient être écrasées."
    echo
    echo "Souhaitez-vous continuer le pull malgré tout ? (o/N)"
    read -r reponse

    if [[ "$reponse" != "o" && "$reponse" != "O" ]]; then
        echo "❌ Opération annulée."
        exit 1
    fi
fi

# Lance le pull
echo "📥 Pull de la branche 'main'..."
git pull origin main

# Synchronise tous les tags (même ceux pas pointés par des commits de 'main')
echo "🏷️  Synchronisation des tags..."
git fetch --tags

echo "✅ Pull et mise à jour des tags terminés."

echo "Appuyez sur une touche pour continuer..."
read -n 1 -s
