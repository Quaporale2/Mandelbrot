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
echo "📥 Pull en cours..."
git pull origin main --tags
