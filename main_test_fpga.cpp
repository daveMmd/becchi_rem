//
// Created by 钟金诚 on 2020/7/13.
//

#include "stdinc.h"
#include "nfa.h"
#include "dfa.h"
#include "ecdfa.h"
#include "hybrid_fa.h"
#include "parser.h"
#include "trace.h"
#include "Fb_DFA.h"

/*
 * Program entry point.
 * Please modify the main() function to add custom code.
 * The options allow to create a DFA from a list of regular expressions.
 * If a single single DFA cannot be created because state explosion occurs, then a list of DFA
 * is generated (see MAX_DFA_SIZE in dfa.h).
 * Additionally, the DFA can be exported in proprietary format for later re-use, and be imported.
 * Moreover, export to DOT format (http://www.graphviz.org/) is possible.
 * Finally, processing a trace file is an option.
 */


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

void calc_range_edge(DFA* dfa){
    state_t ** stt = dfa->get_state_table();
    int dfa_size = dfa->size();
    for(int i=0; i<dfa_size; i++){
        printf("*****state:%d*****\n", i);
        int pre = 0;
        for(int j=1; j<256; j++){
            if(stt[i][j] != stt[i][j-1]){
                printf("%d-%d:%d\n", pre, j-1, stt[i][j-1]);
                pre = j;
            }
        }
        printf("%d-%d:%d\n", pre, 255, stt[i][255]);
    }
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
    DEBUG=config.debug;
    if (DEBUG) VERBOSE=1;

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

    // if regex file is provided, parses it and instantiate the corresponding NFA.
    // if feasible, convert the NFA to DFA
    if (config.regex_file!=NULL){
        FILE *regex_file=fopen(config.regex_file,"r");
        fprintf(stderr,"\nParsing the regular expression file %s ...\n",config.regex_file);
        regex_parser *parse=new regex_parser(config.i_mod,config.m_mod);
        list<NFA *>* nfa_list = NULL;
        int size;
        printf("parsing res to NFAs.\n");
        nfa_list = parse->parse_to_list_mp(regex_file, &size);
        fclose(regex_file);
        delete parse;

        printf("NFA2DFAing...\n");
        //vector<int> dfa_sizes;
        FILE* ftarge = fopen("./static.txt", "w");
        vector<pair<int, char*>> dfa_sizes_patterns;
        for(int i = 0; i < size; i++){
            printf("%d/%d\n", i, size);
            nfa = nfa_list->front();
            nfa_list->pop_front();
            nfa->remove_epsilon();
            nfa->reduce();
            dfa=nfa->nfa2dfa();
            if (dfa==NULL) printf("Max DFA size %ld exceeded during creation: the DFA was not generated\n",MAX_DFA_SIZE);
            else {
                dfa->minimize();
                //calc_range_edge(dfa);
                printf("now processing re: %s\n", nfa->pattern);
                printf("8-bit DFA size:%d\n", dfa->size());

                //debug
                /*FILE* file = fopen("./8dfa.dot", "w");
                dfa->to_dot(file, "8dfa");
                fclose(file);*/

                //dfa_sizes.push_back(dfa->size());
                Fb_DFA* fdfa = new Fb_DFA(dfa);
                //fdfa->to_dot("./4dfa.dot", "4dfa"); //debug
                Fb_DFA* minimum_dfa = fdfa->minimise();
                printf("4-bit DFA size:%d\n", minimum_dfa->size());
                //minimum_dfa->to_dot("./4dfa-m.dot", "4dfa");
                int cnt_less2 = minimum_dfa->less2states();
                int cnt_cons2 = minimum_dfa->cons2states();
                fprintf(ftarge, "\n%d %d %d %d\n%s", dfa->size(), minimum_dfa->size(), cnt_less2, cnt_cons2, nfa->pattern);
                delete fdfa;
                delete minimum_dfa;
                dfa_sizes_patterns.push_back(make_pair(dfa->size(), nfa->pattern));
            }
            delete nfa;
            if(dfa != NULL) delete dfa;
        }
        fclose(ftarge);
        //sort dfa_sizes
        //sort(dfa_sizes.begin(), dfa_sizes.end());
#if 0
        sort(dfa_sizes_patterns.begin(), dfa_sizes_patterns.end(), greater<pair<int, char*>>());
        for(int i = 0; i < size; i++){
            //printf("%d\n", dfa_sizes[i]);
            printf("%d %s\n", dfa_sizes_patterns[i].first, dfa_sizes_patterns[i].second);
        }
#endif
    }

    return 0;

}

