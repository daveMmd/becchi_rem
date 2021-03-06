
#include "rcdfa.h"

RCDFA::RCDFA(unsigned N) : DFA(N){

}

RCDFA::RCDFA(DFA* dfa) : DFA(dfa->size()){
	delete this->state_table;
	this->state_table = NULL;

	_size = dfa->size();
	linked_set** o_accepted_rules = dfa-> get_accepted_rules();
	state_t ** state_table = dfa->get_state_table();

	for(state_t s=0;s<_size;++s){
		accepted_rules[s] = new linked_set();
		accepted_rules[s]->add(o_accepted_rules[s]);
	}

	range_edges_table = (list_re **)allocate_array(_size,sizeof(list_re*));

	int *mark = allocate_int_array(CSIZE);
	RangeEdge* r_edge = NULL;
	state_t target=NO_STATE;
	for (state_t s=0;s<_size;s++){
		for(int i=0;i<CSIZE;i++) mark[i]=0;

		list_re * re_list = new list_re();
		range_edges_table[s] = re_list;
		for (int c=0;c<CSIZE;c++){
			if (!mark[c]){
				mark[c]=1;
				if (state_table[s][c]!=0){
					target=state_table[s][c];
					r_edge = new RangeEdge();
					r_edge->start = c;
					r_edge->end = c;
					r_edge->target = target;

					bool range=true;
					int begin_range=c;
					for(int d=c+1;d<CSIZE;d++){
						if(state_table[s][d]==target){
							// printf("\t%d -> %d = %d\n", s, d, target);
							mark[d] = 1;
							if(!range){
								range = true;
								r_edge = new RangeEdge();
								r_edge->start = d;
								r_edge->end = d;
								r_edge->target = target;
							}
						}

						if(range && (state_table[s][d]!=target || d==CSIZE-1)){
							range = false;
							if(state_table[s][d]!=target){
								r_edge->end = d-1;
							}else{
								r_edge->end = d;
							}
							re_list->push_back(r_edge);
						}
					}
				}
			}
		}
	}
	free(mark);

	this->range_edges_lists = (RangeEdgeList*)allocate_array(_size, sizeof(RangeEdgeList));
	for (state_t s=0;s<_size;s++){
		list_re * re_list = range_edges_table[s];
		this->range_edges_lists[s].lenght = re_list->size();
		this->range_edges_lists[s].ranges = (RangeEdge*)allocate_array(re_list->size(), sizeof(RangeEdge));
		unsigned int index = 0;
		for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){
			this->range_edges_lists[s].ranges[index++] = **it;
		}
	}
}

RCDFA::~RCDFA() {
	for (state_t s=0;s<_size;s++){
		list_re * re_list = range_edges_table[s];
		for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){
			free(*it);
		}
		free(re_list);
	}
	delete range_edges_table;

}

int RCDFA::match(FILE *file){
  rewind(file);
  state_t current = 0;
  unsigned int c = fgetc(file); 
  while(c!=EOF){
    list_re * re_list = range_edges_table[current]; //one memory access
  	int finded = 0;
  	for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){ //one memory access
  		if(c >= (*it)->start && c <= (*it)->end){ //one (or more) memory access
  			current = (*it) -> target;
  			finded = 1;
  			break;
  		}
  	}
  	if(!finded){
  		current = 0;
  	}
    if(!accepted_rules[current]->empty()){
      return 1;
    }
    c = fgetc(file);
  }
  return 0;
}

unsigned int RCDFA::get_m_size(){
	unsigned int m_size = DFA::get_m_size();
	unsigned int range_sum = 0;
	if(DEBUG) printf("#\tcount\tsingleCount\ttlenghtAvg\n");
	for (state_t s=0;s<_size;s++){
		list_re * re_list = range_edges_table[s];
		range_sum += re_list->size();
		unsigned int lenghtSum = 0;
		unsigned int singleCount = 0;
		for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){
			unsigned int lenght = (*it)->end - (*it)->start + 1;
			lenghtSum += lenght;
			if(lenght==1){
				singleCount++;
			}
		}

		if(DEBUG) printf("#%d\t%d\t%d\t%lf\n", s, re_list->size(), singleCount, 1.0*lenghtSum/re_list->size());
		m_size += sizeof(list_re) + re_list->size() *(16+ sizeof(RangeEdge*) + sizeof(RangeEdge)) + 24;
	}
	if(DEBUG) printf("sum:%d\n", range_sum);
	return m_size;
}

uint32_t RCDFA::get_real_mem_size(){
    unsigned int sum = 0;
    // printf("%d %d\n", sizeof(RangeEdgeList), sizeof(RangeEdge));
    sum = _size * sizeof(RangeEdgeList);
    for (state_t s=0; s<_size; s++){
        RangeEdgeList re_list = range_edges_lists[s];
        sum += re_list.lenght * 4;
    }
    return sum;
}

unsigned int RCDFA::getMemSize() {
	unsigned int sum = 0;
	// printf("%d %d\n", sizeof(RangeEdgeList), sizeof(RangeEdge));
	sum = _size * sizeof(RangeEdgeList);
	for (state_t s=0; s<_size; s++){
		RangeEdgeList re_list = range_edges_lists[s];
		sum += re_list.lenght* sizeof(RangeEdge);
	}
	return sum;
}

int RCDFA::match(char * str){
  int i = 0;
  state_t current = 0;
  while(str[i]!=0){
  	list_re * re_list = range_edges_table[current];
  	int finded = 0;
  	for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){
  		if(str[i] >= (*it)->start && str[i] <= (*it)->end){
  			current = (*it) -> target;
  			finded = 1;
  			break;
  		}
  	}
  	if(!finded){
  		current = 0;
  	}
    if(!accepted_rules[current]->empty()){
      return 1;
    }
    i++;
  }
  return 0;
}

void RCDFA::to_dot(FILE *file, const char *title){
	// set_depth();
	fprintf(file, "digraph \"%s\" {\n", title);
	for (state_t s=0;s<_size;s++){
		if (accepted_rules[s]->empty()){ 
			fprintf(file, " %ld [shape=circle,label=\"%ld",s,s);
			fprintf(file,"-%ld\"];\n", 1);
			//fprintf(file, " %ld [shape=circle];\n", s);
		}else{ 
			fprintf(file, " %ld [shape=doublecircle,label=\"%ld",s,s);
			fprintf(file, "-%ld/", 1);
			linked_set *ls=	accepted_rules[s];
			while(ls!=NULL){
				if(ls->succ()==NULL)
					fprintf(file,"%ld",ls->value());
				else
					fprintf(file,"%ld,",ls->value());
				ls=ls->succ();
			}
			fprintf(file,"\"];\n");
		}
	}
	char *label=NULL;
	char *temp=(char *)malloc(100);
	state_t target=NO_STATE;
	for (state_t s=0;s<_size;s++){
		list_re * re_list = range_edges_table[s];
		for(list_re::iterator it= re_list->begin();it!=re_list->end();++it){
			char c;
			label=(char *)malloc(100);
			c = (*it)->start;
			if ((c>='a' && c<='z') || (c>='A' && c<='Z')) {
				sprintf(label,"%c",c);
			}
			else{
				sprintf(label,"%d",c);
			}
			if(c != (*it)->end){
				c = (*it)->end;
				if ((c>='a' && c<='z') || (c>='A' && c<='Z')) {
					sprintf(temp,"-%c",c);
				}
				else{
					sprintf(temp,"-%d",c);
				}
				label=strcat(label,temp);
			}
			fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s,(*it)->target,label);	
			free(label);
		}
	}
	free(temp);
	fprintf(file,"}");
}