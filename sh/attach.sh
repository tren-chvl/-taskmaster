#!/bin/bash

echo "=================================="
echo "   Test Taskmaster - Attach/Detach"
echo "=================================="
echo "PID du programme : $$"
echo "Tape quelque chose puis appuie sur Entrée."
echo "Le programme tourne en boucle."
echo

count=1

while true
do
    echo "[$(date '+%H:%M:%S')] Message numéro $count"
    count=$((count + 1))

    if read -r -t 5 -p "Commande > " input
    then
        echo "Tu as tapé : $input"

        if [ "$input" = "stop-test" ]
        then
            echo "Arrêt du programme de test."
            exit 0
        fi
    fi
done
