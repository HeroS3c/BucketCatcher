#!/bin/bash
echo -e "\e[1;31m REMEBEMBER TO RUN THIS SCRIPT AS SUDO \e[0m"
sudo apt-get install sqlite3 || sudo pacman -S sqlite3 || yum install sqlite3 || dnf install sqlite3 #install sqlite3
sudo apt-get install node || sudo pacman -S node || yum install node || dnf install node #install nodejs
make # compile 
echo -e "\e[0;36m Ready to go. \e[0m"
