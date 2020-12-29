#include <fstream>

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "parser.h"
#include "Fb_DFA.h"
#include "./component_tree/util.h"
#include "hierarMerging.h"

int DEBUG = 0;
int VERBOSE = 0;
//#define COMPILE_INTO_ONE_NFA
/////////////////////////////////////////////////////////////
// gps coordinate
//
// illustrates serialization for a simple type
//
class gps_position
{
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & degrees;
        ar & minutes;
        ar & seconds;
    }
    int degrees;
    int minutes;
    float seconds;
    char * s;
public:
    gps_position(){};
    gps_position(int d, int m, float s) :
            degrees(d), minutes(m), seconds(s)
    {}
};

void test_serialization(){
    // create and open a character archive for output
    std::ofstream ofs("filename");

    // create class instance
    const gps_position g(35, 59, 24.567f);

    // save data to archive
    {
        boost::archive::text_oarchive oa(ofs);
        // write class instance to archive
        oa << g;
        // archive and stream closed when destructors are called
    }

    // ... some time later restore the class instance to its orginal state
    gps_position newg;
    {
        // create and open an archive for input
        std::ifstream ifs("filename");
        boost::archive::text_iarchive ia(ifs);
        // read class state from archive
        ia >> newg;
        // archive and stream closed when destructors are called
    }
}

int main(int argc, char** argv) {
    char* rulefile;
    if(argc > 1) rulefile = argv[1];

    auto parser = regex_parser(false, false);
#ifdef COMPILE_INTO_ONE_NFA
    FILE* f = fopen(rulefile, "r");
    printf("Generating NFA...\n");
    NFA* nfa = parser.parse(f);
    printf("Generating DFA...\n");
    DFA* dfa = nfa->nfa2dfa();

    printf("Minimising DFA...\n");
    dfa->minimize();
    printf("generating NFA...\n");
    DFA *one_accept_dfa = dfa->minimize_do_not_dis_accept();

    auto* fb_dfa = new Fb_DFA(dfa);
    Fb_DFA* minimise_fb_dfa = fb_dfa->minimise2();

    /*FILE *fp = fopen("./dfa.dot", "w");
    dfa->to_dot(fp, "dfa");
    fclose(fp);

    fp = fopen("./one_accept_dfa.dot", "w");
    one_accept_dfa->to_dot(fp, "one_accept_dfa");
    fclose(fp)

    minimise_fb_dfa->to_dot("./fb_dfa.dot", "minimise_fb_dfa");*/

    printf("nfa size: %d\ndfa size: %d\n", nfa->size(), dfa->size());
    printf("one_accept_dfa size:%d\n", one_accept_dfa->size());
    printf("fb_dfa size: %d\nminimise fb_dfa size: %d\n", fb_dfa->size(), minimise_fb_dfa->size());
#else
    list<char *>* regex_list = read_regexset(rulefile);
    Fb_DFA* final_fbdfa = nullptr;
    uint16_t re_id = 0;
    auto *dfalis = new list<DFA*>();
    for(auto &re: *regex_list){
        NFA* nfa = parser.parse_from_regex(re);
        DFA* dfa = nfa->nfa2dfa();
        dfa->minimize();
        dfalis->push_back(dfa);
        /*auto* fb_dfa = new Fb_DFA(dfa, re_id++);
        Fb_DFA* minimise_fb_dfa = fb_dfa->minimise2();
        delete fb_dfa;
        if(final_fbdfa) {
            Fb_DFA* tem_fbdfa = final_fbdfa->converge(minimise_fb_dfa);
            delete final_fbdfa;
            delete minimise_fb_dfa;
            final_fbdfa = tem_fbdfa;
        }
        else{
            final_fbdfa = minimise_fb_dfa;
        }*/
    }
    DFA* dfa = hm_dfalist2dfa(dfalis);
    DFA* one_accept_dfa = dfa->minimize_do_not_dis_accept();
    printf("dfa size: %d\n", dfa->size());
    printf("one_accept_dfa size:%d\n", one_accept_dfa->size());

    //Fb_DFA* final_minimise_fbdfa = final_fbdfa->minimise2();
    //printf("final_fbdfa size: %d\nminimise final_fbdfa size: %d\n", final_fbdfa->size(), final_minimise_fbdfa->size());

    //final_fbdfa->to_dot("./final_fbdfa.dot", "final_fbdfa");
    //final_minimise_fbdfa->to_dot("./final_minimise_fbdfa.dot", "final_minimise_fbdfa");
#endif
    return 0;
}
