#!/bin/bash

#set -x

PAGELIST=pagelist2


URL=http://wiki.eressea.de/index.php?title=
RAWACTION="&action=raw"

RAW_DIR=mediawiki
JSON_DIR=json
DOCBOOK_DIR=docbook
RST_DIR=rst
HTML_DIR=html
LATEX_DIR=latex

DOC_EXT=.xml
LOG_EXT=.log
JSON_EXT=.json
WIKI_EXT=.mediawiki
RST_EXT=.rst
HTML_EXT=.html

DO_RAW=0
DO_JSON=0
DO_HTML0=0
DO_RST=0
DO_DOCBOOK=0
DO_CONCAT=0
DO_HTML1=0
DO_PDF=1

PAPER_SIZE=a4

main_title="Eressea-Regeln"

verbose=1

function check_file {
    if [ ! -r "$1" ]; then
	echo "File $1 is not readable, aborting."
	exit -1
    fi
}

function check_dir {
    if [ ! -d "$1" ]; then
	echo "Directory $1 does not exist, aborting."
	exit -1
    fi
}

function link_id {
    id=$(echo -n "$1" | tr -c -s '[a-z][A-Z][0-9][_.-]' '_')
    python -c "from ewiki import link_id; print link_id('', '$id')"
}

function url_encode {
    python -c "from ewiki import url_encode; print url_encode('$1')"
}

function html_encode {
    python -c "from ewiki import html_encode; print html_encode(u'$1')"
}

if ((DO_RAW==1)); then
    if ((verbose>0)); then
 	echo "downloading mediawiki into $RAW_DIR"
    fi
    check_dir $RAW_DIR
fi

if ((DO_JSON==1)); then
    if ((verbose>0)); then
 	echo "creating raw json in $JSON_DIR"
    fi
    check_dir $JSON_DIR
fi

if ((DO_DOCBOOK==1)); then
    if ((verbose>0)); then
 	echo "creating docbook in $DOCBOOK_DIR"
    fi
    check_dir $DOCBOOK_DIR
fi

if ((DO_HTML0==1)); then
    if ((verbose>0)); then
 	echo "creating html docs in $HTML_DIR"
    fi
    check_dir $HTML_DIR
fi

if ((DO_HTML0==1)); then
    if ((verbose>0)); then
 	echo "creating big html in $HTML_DIR"
    fi
    check_dir $HTML_DIR
fi

if ((DO_RST==1)); then
    if ((verbose>0)); then
 	echo "creating rst docs in $RST_DIR"
    fi
    check_dir $RST_DIR
fi

concat_file="$DOCBOOK_DIR/allpages.xml"

if ((DO_CONCAT==1)); then
    if [ -e "$concat_file" ]; then
	echo "$concat_file exists, aborting."
	exit -1
    fi
    touch "$concat_file"
fi

html0_contentsfile=$HTML_DIR/Inhalt$HTML_EXT
html0_templatefile=wikitemplate0.html
html1_templatefile=wikitemplate0.html
html1_tmpfile=$HTML_DIR/html1_tmp$HTML_EXT
html1_file=$HTML_DIR/Regeln_komplett$HTML_EXT

latex_file=$LATEX_DIR/Regeln_komplett.tex

if ((DO_HTML0==1)); then
    echo "<html><head>"  > "$html0_contentsfile"
    echo "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"  >> "$html0_contentsfile"
    echo "<title>$main_title: Inhalt</title>" >> "$html0_contentsfile"
    echo "</head><body>" >> "$html0_contentsfile"
    echo "<b>Inhalt</b><br /><div id='content'><ul>" >> "$html0_contentsfile"
fi

if ((DO_HTML1==1)); then
    rm -f "$html1_tmpfile"
fi


cat $PAGELIST | while read pageline; do
    if [ -z "$pageline" ]; then
	continue
    fi
    if [ ${pageline:0:1} = "#" ]; then
	echo "skipping $pageline"
	continue;
    fi
    IFS='	' read -r -a linearray <<< "$pageline"
    depth=${linearray[0]}
    page=${linearray[1]}
    pageurl="$(perl -MURI::Escape -e 'print uri_escape($ARGV[0]);' "$page")"

    title=${linearray[2]}
    if [ -z "$title" ]; then
	title=$page
    fi

    page_id=${linearray[3]}
    if [ -z "$page_id" ]; then
	page_id=$page
    fi
    page_id=$(link_id "$page_id")
    
    echo "processing $pageurl at depth $depth, named '$title' ($page_id)"

    rawfile=$RAW_DIR/$page$WIKI_EXT
    infile=$RAW_DIR/$page$WIKI_EXT
    jsonfile=$JSON_DIR/$page$JSON_EXT
    rstfile=$RST_DIR/$page$RST_EXT
    htmlfile=$HTML_DIR/$(url_encode "$page$HTML_EXT")
    outputfile=$DOCBOOK_DIR/$page$DOC_EXT
    
    if ((DO_RAW==1)); then
	check_dir $RAW_DIR
	#wget -o "$RAW_DIR/$page$LOG_EXT" --output-document="$rawfile" "$URL$pageurl$RAWACTION"
        curl -s -f -m60 -o "$rawfile" "$URL$pageurl$RAWACTION"
        status=$?
        if (($status!=0)); then
            echo "error downloading $page, exit code $status."
        fi
	# insert h1 header
	# sed -i 1i"= $title =\n" "$rawfile"
    fi

    if ((DO_JSON==1)); then
	check_file "$infile"
	pandoc -f mediawiki -t json -o "$jsonfile" "$infile" 
    fi

    if ((DO_HTML0==1)); then
	cat "$jsonfile" | 
	  pandoc -f json -t html -M id-prefix="$page_id." --filter=ewiki_filter_html0.py --id-prefix="$page_id." -s -V title="$title" -V pagetitle="$title" -V css="common.css" -V contents="Inhalt.html" --template="$html0_templatefile" --toc > "$htmlfile" 
	url=$(url_encode "$page")
	url=$(url_encode "$url$HTML_EXT")
        echo "<li><a href="$url">$title</a></li>" >> "$html0_contentsfile"
    fi

    if ((DO_HTML1==1)); then
	echo "<h1 id=\"$page_id.\">$title</h1>" >> "$html1_tmpfile"
	cat "$jsonfile" | 
	pandoc -f json -t html -M id-prefix="$page_id." --filter=ewiki_filter_html1.py --id-prefix="$page_id." >> "$html1_tmpfile"
	echo "" >> "$html1_tmpfile"
    fi

    if ((DO_RST==1)); then
	cat "$jsonfile" | 
	  python ewiki_filter.py | 
	  pandoc -f json -t rst --id-prefix="$page_id." -s --template=eresseawikirst.template --base-header-level=$((depth+1)) -V title="$title"  > "$rstfile" 
    fi

    if ((DO_DOCBOOK==1)); then
	cat "$jsonfile" | 
	  python ewiki_filter.py | 
	  pandoc -f json -t docbook -s --id-prefix="$page_id." --template=eresseawikidoc.template --base-header-level=$((depth+1)) -V title="$title" |
	  # replace sect1 by section
	  sed -e "s/sect[0-9]/section/" > "$outputfile" 
#	pandoc -f mediawiki -t docbook -s --id-prefix="$page_id." --template=eresseawikidoc.template --base-header-level=$((depth+1)) -V title="$title" -o "$outputfile" "$infile"
    fi

    if ((DO_CONCAT==1)); then
	check_file "$outputfile"
	cat "$outputfile" >> "$concat_file"
    fi
done

if ((DO_HTML0==1)); then
    echo "</ul></body></html>" >> "$html0_contentsfile"
fi

if ((DO_PDF==1)); then
    echo "creating tex file"
    check_file "$html1_tmpfile"
    cat "$html1_tmpfile" |
    pandoc -f html -t latex -s -M title="$main_title" -V title="$main_title" -V linkcolor=blue -V lot=true -V papersize=$PAPER_SIZE  --toc --toc-depth=2 --id-prefix="ewiki."  > "$latex_file"
    status=$?
    echo "creating pdf file"
    if (($status==0)); then    
	pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null && pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null && pdflatex --output-directory "$LATEX_DIR" -halt-on-error "$latex_file" > /dev/null
    fi
fi

if ((DO_HTML1==1)); then
    cat "$html1_tmpfile" |
    pandoc -f html -t html -M id-prefix="$page_id." -s -V title="$main_title" -V pagetitle="$main_title" -V css="common.css" --template="$html1_templatefile" --toc --toc-depth=2 > "$html1_file"
    # rm -f "$html_tmpfile"
fi


# works:
# mediawiki2latex -o eresseawiki.pdf -c eresseatree -u http://localhost/testwiki/index.php/Regeln_\(komplett\)
