//
// Created by 钟金诚 on 2020/5/16.
// main entry file to test hfa (change the construct)

#include <sys/time.h>
#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "ecdfa.h"
#include "hybrid_fa.h"
#include "parser.h"
#include "trace.h"
#include "hfadump.h"

#ifndef CUR_VER
#define CUR_VER		"Michela  Becchi 1.4.1"
#endif

int VERBOSE;
int DEBUG;

/*
 * Returns the current version string
 */
void version(){
    printf("version:: %s\n", CUR_VER);
}

/* usage */
static void usage()
{
    fprintf(stderr,"\n");
    fprintf(stderr, "Usage: regex [options]\n");
    fprintf(stderr, "             [--parse|-p <regex_file> [--m|--i] | --import|-i <in_file> ]\n");
    fprintf(stderr, "             [--export|-e  <out_file>][--graph|-g <dot_file>]\n");
    fprintf(stderr, "             [--trace|-t <trace_file>]\n");
    fprintf(stderr, "             [--hfa]\n\n");
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "    --help,-h       print this message\n");
    fprintf(stderr, "    --version,-r    print version number\n");
    fprintf(stderr, "    --verbose,-v    basic verbosity level \n");
    fprintf(stderr, "    --debug,  -d    enhanced verbosity level \n");
    fprintf(stderr, "\nOther:\n");
    fprintf(stderr, "    --parse,-p <regex_file>  process regex file\n");
    fprintf(stderr, "    --m,--i  m modifier, ignore case\n");
    fprintf(stderr, "    --import,-i <in_file>    import DFA from file\n");
    fprintf(stderr, "    --export,-e <out_file>   export DFA to file\n");
    fprintf(stderr, "    --graph,-g <dot_file>    export DFA in DOT format into specified file\n");
    fprintf(stderr, "    --trace,-t <trace_file>  trace file to be processed\n");
    fprintf(stderr, "    --hfa                    generate the hybrid-FA\n");
    fprintf(stderr, "\n");
    exit(0);
}

/* configuration */
static struct conf {
    char *regex_file;
    char *in_file;
    char *out_file;
    char *dot_file;
    char *trace_file;
    bool i_mod;
    bool m_mod;
    bool verbose;
    bool debug;
    bool hfa;
} config;

/* initialize the configuration */
void init_conf(){
    config.regex_file=NULL;
    config.in_file=NULL;
    config.out_file=NULL;
    config.dot_file=NULL;
    config.trace_file=NULL;
    config.i_mod=false;
    config.m_mod=false;
    config.debug=false;
    config.verbose=false;
    config.hfa=false;
}

/* print the configuration */
void print_conf(){
    fprintf(stderr,"\nCONFIGURATION: \n");
    if (config.regex_file) fprintf(stderr, "- RegEx file: %s\n",config.regex_file);
    if (config.in_file) fprintf(stderr, "- DFA import file: %s\n",config.in_file);
    if (config.out_file) fprintf(stderr, "- DFA export file: %s\n",config.out_file);
    if (config.dot_file) fprintf(stderr, "- DOT file: %s\n",config.dot_file);
    if (config.trace_file) fprintf(stderr,"- Trace file: %s\n",config.trace_file);
    if (config.i_mod) fprintf(stderr,"- ignore case selected\n");
    if (config.m_mod) fprintf(stderr,"- m modifier selected\n");
    if (config.verbose && !config.debug) fprintf(stderr,"- verbose mode\n");
    if (config.debug) fprintf(stderr,"- debug mode\n");
    if (config.hfa)   fprintf(stderr,"- hfa generation invoked\n");
}

/* parse the main call parameters */
static int parse_arguments(int argc, char **argv)
{
    int i=1;
    if (argc < 2) {
        usage();
        return 0;
    }
    while(i<argc){
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
            usage();
            return 0;
        }else if(strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--version") == 0){
            version();
            return 0;
        }else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0){
            config.verbose=1;
        }else if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0){
            config.debug=1;
        }else if(strcmp(argv[i], "--hfa") == 0){
            config.hfa=1;
        }else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--graph") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Dot file name missing.\n");
                return 0;
            }
            config.dot_file=argv[i];
        }else if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--import") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Import file name missing.\n");
                return 0;
            }
            config.in_file=argv[i];
        }else if(strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--export") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Export file name missing.\n");
                return 0;
            }
            config.out_file=argv[i];
        }else if(strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--parse") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Regular expression file name missing.\n");
                return 0;
            }
            config.regex_file=argv[i];
        }else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
            i++;
            if(i==argc){
                fprintf(stderr,"Trace file name missing.\n");
                return 0;
            }
            config.trace_file=argv[i];
        }else if(strcmp(argv[i], "--m") == 0){
            config.m_mod=true;
        }else if(strcmp(argv[i], "--i") == 0){
            config.i_mod=true;
        }else{
            fprintf(stderr,"Ignoring invalid option %s\n",argv[i]);
        }
        i++;
    }
    return 1;
}

/* check that the given file can be read/written */
void check_file(char *filename, char *mode){
    FILE *file=fopen(filename,mode);
    if (file==NULL){
        fprintf(stderr,"Unable to open file %s in %c mode",filename,mode);
        fatal("\n");
    }else fclose(file);
}


/*
 *  MAIN - entry point
 */
int main(int argc, char **argv){

    //read configuration
    init_conf();
    while(!parse_arguments(argc,argv)) usage();
    print_conf();
    VERBOSE=config.verbose;
    DEBUG=config.debug; if (DEBUG) VERBOSE=1;

    //check that it is possible to open the files
    if (config.regex_file!=NULL) check_file(config.regex_file,"r");
    if (config.in_file!=NULL) check_file(config.in_file,"r");
    if (config.out_file!=NULL) check_file(config.out_file,"w");
    if (config.dot_file!=NULL) check_file(config.dot_file,"w");
    if (config.trace_file!=NULL) check_file(config.trace_file,"r");

    // check that either a regex file or a DFA import file are given as input
    if (config.regex_file==NULL && config.in_file==NULL){
        fatal("No data file - please use either a regex or a DFA import file\n");
    }
    if (config.regex_file!=NULL && config.in_file!=NULL){
        printf("DFA will be imported from the Regex file. Import file will be ignored");
    }

    /* FA declaration */
    NFA *nfa=NULL;  	// NFA
    DFA *dfa=NULL;		// DFA
    HybridFA *hfa=NULL; // Hybrid-FA

    // if regex file is provided, parses it and instantiate the corresponding NFA.
    // if feasible, convert the NFA to DFA
    if (config.regex_file!=NULL){
        FILE *regex_file=fopen(config.regex_file,"r");
        fprintf(stderr,"\nParsing the regular expression file %s ...\n",config.regex_file);
        regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);

        struct timeval start,end;
        gettimeofday(&start, NULL);

        int no_use;
        printf("before parse_to_list_mp\n");
        nfa_list *ptr_nfalist = parse->parse_to_list_mp(regex_file, &no_use);

        gettimeofday(&end, NULL);
        long timeuse =1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
        printf("regexs to nfa time=%f\n",timeuse /1000000.0);

        fclose(regex_file);
        delete parse;
        printf("before new hybridFA\n");
        hfa = new HybridFA(ptr_nfalist);

        gettimeofday(&end, NULL);
        timeuse =1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
        printf("finish constructing hfa, cost time=%f\n",timeuse /1000000.0);

        //dump hfa
        hfa->dumpmem("./hfa.mem");
        //free nfa-list
        delete ptr_nfalist;
        delete hfa;
    }

    read_mem("./hfa.mem");

    unsigned char teststring[50] = "abcdabcdaefgsdsdwfabfg";
    mem_search_hfa(teststring, 50);
    //exit(0);

    //test init hfa
    if (config.regex_file!=NULL){
        FILE *regex_file=fopen(config.regex_file,"r");
        fprintf(stderr,"\nParsing the regular expression file %s ...\n",config.regex_file);
        regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);

        struct timeval start,end;
        gettimeofday(&start, NULL);

        NFA *nfa = parse->parse(regex_file);
        fclose(regex_file);
        delete parse;
        hfa = new HybridFA(nfa);

        gettimeofday(&end, NULL);
        long timeuse =1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
        printf("time=%f\n",timeuse /1000000.0);
        //traverse
        trace *tr=new trace("./test_trace");
        tr->traverse(hfa);

        //free nfa-list
        delete nfa;
        delete hfa;
    }

    exit(0);

    // HFA generation
    if (config.hfa){
        if (nfa==NULL) fatal("Impossible to build a Hybrid-FA if no NFA is given.");
        hfa=new HybridFA(nfa);
        if (hfa->get_head()->size()<100000) hfa->minimize();
        printf("HFA:: head size=%d, tail size=%d, number of tails=%d, border size=%d\n",hfa->get_head()->size(),hfa->get_tail_size(),hfa->get_num_tails(),hfa->get_border()->size());
    }

    // trace file traversal
    if (config.trace_file){
        trace *tr=new trace(config.trace_file);
        if (nfa!=NULL) tr->traverse(nfa);
        if (dfa!=NULL){
            tr->traverse(dfa);
            if (dfa->get_default_tx()!=NULL) tr->traverse_compressed(dfa);
        }
        if (hfa!=NULL) tr->traverse(hfa);
        delete tr;
    }

    /* Automata de-allocation */

    if (nfa!=NULL) delete nfa;
    if (dfa!=NULL) delete dfa;
    if (hfa!=NULL) delete hfa;

    return 0;

}

