@echo off
set REPO_URL=REPO_URL_HERE
git init
git branch -M main
git remote add origin %REPO_URL%
git add .
git commit -m "Initial scaffold: Coup assignment v4"
git push -u origin main
