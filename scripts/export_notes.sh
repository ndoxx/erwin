#!/bin/sh

pandoc --latex-engine=xelatex ../NOTES.md --reference-links --toc -V geometry:"left=2cm, top=2cm, right=2cm, bottom=2cm" -V fontsize=10pt -s -o ../notes.pdf