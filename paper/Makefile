.PHONY: all clean

all: paper.pdf

paper.pdf: *.tex *.bib *.cls
	pdflatex paper; bibtex paper; pdflatex paper; pdflatex paper;

clean:
	rm *.out *.log *.bbl *.aux *.blg *.toc paper.pdf

