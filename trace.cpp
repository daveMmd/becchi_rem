/*
 * Copyright (c) 2007 Michela Becchi and Washington University in St. Louis.
 * All rights reserved
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. The name of the author or Washington University may not be used
 *       to endorse or promote products derived from this source code
 *       without specific prior written permission.
 *    4. Conditions of any other entities that contributed to this are also
 *       met. If a copyright notice is present from another entity, it must
 *       be maintained in redistributions of the source code.
 *
 * THIS INTELLECTUAL PROPERTY (WHICH MAY INCLUDE BUT IS NOT LIMITED TO SOFTWARE,
 * FIRMWARE, VHDL, etc) IS PROVIDED BY  THE AUTHOR AND WASHINGTON UNIVERSITY
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR WASHINGTON UNIVERSITY
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS INTELLECTUAL PROPERTY, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * */

/*
 * File:   trace.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 */

#include "trace.h"
#include "dheap.h"
#include <dirent.h>
#include <set>
#include <iterator>
#include <iostream>
#include "parse_pcap.h"

using namespace std;

Packet **pkts = nullptr;

trace::trace(char *filename){
	tracefile=NULL;
	if (filename!=NULL) set_trace(filename);
	else tracename=NULL;
}
	
trace::~trace(){
	if (tracefile!=NULL) fclose(tracefile);
}

void trace::set_trace(char *filename){
	if (tracefile!=NULL) fclose(tracefile);
	tracename=filename;
	tracefile=fopen(tracename,"r");
	if (tracefile==NULL) fatal("trace:: set_trace: error opening trace-file\n");
}
	
void trace::traverse(DFA *dfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse DFA on file %s\n...",tracename);
	
	if (dfa->get_depth()==NULL) dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	
	state_t state=0;
	int c=fgetc(tracefile);
	long inputs=0;
	
	unsigned int *stats=allocate_uint_array(dfa->size());
	for (int j=1;j<dfa->size();j++) stats[j]=0;
	stats[0]=1;	
	linked_set *accepted_rules=new linked_set();
	
	while(c!=EOF){
		state=dfa->get_next_state(state,(unsigned char)c);
		stats[state]++;
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		inputs++;
		c=fgetc(tracefile);
	}
	fprintf(stream,"\ntraversal statistics:: [state #, depth, # traversals, %%time]\n");
	int num=0;
	for (int j=0;j<dfa->size();j++){
		if(stats[j]!=0){
			fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());	
	free(stats);		
	delete accepted_rules;
}

void trace::traverse(EgCmpDfa *ecdfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse ECDFA on file %s\n...",tracename);
	
	// if (dfa->get_depth()==NULL) dfa->set_depth();
	// unsigned int *dfa_depth=dfa->get_depth();
	
	state_t state=0;
	int c=fgetc(tracefile);
	long inputs=0;
	
	unsigned int *stats=allocate_uint_array(ecdfa->getSize());
	for (int j=1;j<ecdfa->getSize();j++) stats[j]=0;
	stats[0]=1;	
	linked_set *accepted_rules=new linked_set();
	
	while(c!=EOF){
		state=ecdfa->getNext(state, (unsigned char) c);
		stats[state]++;
		if (!ecdfa->accepts(state)->empty()){
			accepted_rules->add(ecdfa->accepts(state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=ecdfa->accepts(state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		inputs++;
		c=fgetc(tracefile);
	}
	fprintf(stream,"\ntraversal statistics:: [state #, # traversals, %%time]\n");
	int num=0;
	for (int j=0;j<ecdfa->getSize();j++){
		if(stats[j]!=0){
			fprintf(stream,"[%ld, %ld, %f %%]\n",j,stats[j],(float)stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,ecdfa->getSize(),(float)num*100/ecdfa->getSize());
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());	
	free(stats);		
	delete accepted_rules;
}

void trace::traverse_compressed(DFA *dfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse compressed DFA on file %s\n...",tracename);
	
	if (dfa->get_depth()==NULL) dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	
	unsigned int accesses=0;
	unsigned int *stats=allocate_uint_array(dfa->size()); 	  //state traversals (including the ones due to default transitions)
	unsigned int *dfa_stats=allocate_uint_array(dfa->size()); //state traversals in the original DFA
	for (int j=0;j<dfa->size();j++){
		stats[j]=0;
		dfa_stats[j]=0;
	}
	dfa_stats[0]++;
	stats[0]++;
	
	linked_set *accepted_rules=new linked_set();
	state_t *fp=dfa->get_default_tx();
	int *is_fp=new int[dfa->size()];
	for (state_t s=0;s<dfa->size();s++) is_fp[s]=0;
	for (state_t s=0;s<dfa->size();s++) if(fp[s]!=NO_STATE) is_fp[fp[s]]=1;
	
	unsigned int inputs=0;
	state_t state=0;
	int c=fgetc(tracefile);
	while(c!=EOF){
		state=dfa->lookup(state,c,&stats,&accesses);
		dfa_stats[state]++;
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		inputs++;
		c=fgetc(tracefile);
	}
	fprintf(stream,"\ntraversal statistics:: [state #, depth, #traversal, (# traversals in DFA), %%time]\n");
	fprintf(stream,"                         - state_id*: target of default transition - not taken\n");
	fprintf(stream,"                         - *: target of default transition - taken \n");
	
	int num=0;
	int more_states=0;
	int max_depth=0;
	for (int j=0;j<dfa->size();j++){
		if(stats[j]!=0){
			if (dfa_depth[j]>max_depth) max_depth=dfa_depth[j];
			if (is_fp[j]){
				if (dfa_stats[j]==stats[j])
					fprintf(stream,"[%ld*, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/accesses);
				else{
					fprintf(stream,"[%ld**, %ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],dfa_stats[j],(float)stats[j]*100/accesses);
					more_states++;
				}	
			}
			else fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],stats[j],(float)stats[j]*100/accesses);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());
	fprintf(stream,"fraction traversals %f\n",(float)accesses/inputs);
	fprintf(stream,"number of additional states %ld\n",more_states);
	fprintf(stream,"max depth reached %ld\n",max_depth);	
	free(stats);
	free(dfa_stats);
	delete [] is_fp;		
	delete accepted_rules;
}

void trace::traverse(NFA *nfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse NFA on file %s\n...",tracename);
	
	nfa->reset_state_id(); //reset state identifiers in breath-first order
	
	//statistics
	unsigned int *active=allocate_uint_array(nfa->size()); //active[x]=y if x states are active for y times
	unsigned int *stats=allocate_uint_array(nfa->size()); //stats[x]=y if state x was active y times
	for (int j=0;j<nfa->size();j++){
		stats[j]=0;
		active[j]=0;
	}
	
	//code
	nfa_set *nfa_state=new nfa_set();
	nfa_set *next_state=new nfa_set();
	linked_set *accepted_rules=new linked_set();
	
	nfa_set *closure = nfa->epsilon_closure(); 
	
	nfa_state->insert(closure->begin(),closure->end());
	delete closure;
	
	FOREACH_SET (nfa_state,it) stats[(*it)->get_id()]=1;
	active[nfa_state->size()]=1;
	
	int inputs=0;
	int c=fgetc(tracefile);	
	while(c!=EOF){
		FOREACH_SET(nfa_state,it){
			nfa_set *target=(*it)->get_transitions(c);
			if (target!=NULL){
				FOREACH_SET(target,it2){
					nfa_set *target_closure=(*it2)->epsilon_closure();	
					next_state->insert(target_closure->begin(),target_closure->end());
					delete target_closure;
				}
	            delete target;        	   
			}
		}
		delete nfa_state;
		nfa_state=next_state;
		next_state=new nfa_set();
		active[nfa_state->size()]++;
		FOREACH_SET (nfa_state,it) stats[(*it)->get_id()]++;
		
		linked_set *rules=new linked_set();
		
		FOREACH_SET (nfa_state,it) rules->add((*it)->get_accepting());
				
		if (!rules->empty()){
			accepted_rules->add(rules);
			if (DEBUG){
				char *label=(char *)malloc(100);
				sprintf(label,"%d",rules->value());
				linked_set *acc=rules->succ();
				while(acc!=NULL && !acc->empty()){
					char *tmp=(char *)malloc(5);
					sprintf(tmp,",%d",acc->value());
					label=strcat(label,tmp); free(tmp);
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		delete rules;
		inputs++;
		c=fgetc(tracefile);
	}//end while (traversal)
	
	fprintf(stream,"\ntraversal statistics:: [state #,#traversal, %%time]\n");
	unsigned long num=0;
	for (int j=0;j<nfa->size();j++)
		if(stats[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,stats[j],(float)stats[j]*100/inputs);
			num++;
		}
	fprintf(stream,"%ld out of %ld states traversed (%f %%)\n",num,nfa->size(),(float)num*100/nfa->size());
	fprintf(stream,"\ntraversal statistics:: [size of active state vector #,frequency, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(active[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,active[j],(float)active[j]*100/inputs);
			num+=j*active[j];
		}
	fprintf(stream,"average size of active state vector %f\n",(float)num/inputs);
	fprintf(stream,"rules matched: %ld\n",accepted_rules->size());	
	free(stats);		
	free(active);
	delete nfa_state;
	delete next_state;
	delete accepted_rules;
}

void trace::traverse(HybridFA *hfa, FILE *stream){
	if (tracefile==NULL) fatal("trace file is NULL!");
	rewind(tracefile);
	
	if (VERBOSE) fprintf(stderr, "\n=>trace::traverse Hybrid-FA on file %s\n...",tracename);
	
	NFA *nfa=hfa->get_nfa();
	DFA *dfa=hfa->get_head();
	map <state_t,nfa_set*> *border=hfa->get_border();
	
	//statistics
	unsigned long border_stats=0;  //border states
	
	//here we log how often a head/DFA state is active
	unsigned int *dfa_stats=allocate_uint_array(dfa->size());
	for (int j=1;j<dfa->size();j++) dfa_stats[j]=0;
	
	//here we log how often a tail/NFA state is active and the size of the active set. Only tail states are considered 
	unsigned int *nfa_active=allocate_uint_array(nfa->size()); //active[x]=y if x states are active for y times
	unsigned int *nfa_stats=allocate_uint_array(nfa->size()); //stats[x]=y if state x was active y times
	for (int j=0;j<nfa->size();j++){
		nfa_stats[j]=0;
		nfa_active[j]=0;
	}
	
	//code
	state_t dfa_state=0;
	dfa_stats[0]=1;	
	
	nfa_set *nfa_state=new nfa_set();
	nfa_set *next_state=new nfa_set();
	linked_set *accepted_rules=new linked_set();
	
	int inputs=0;
	int c=fgetc(tracefile);	
	while(c!=EOF){
		/* head-DFA */
		//compute next state and update statistics
		dfa_state=dfa->get_next_state(dfa_state,(unsigned char)c);
		dfa_stats[dfa_state]++;
		//compute accepted rules
		if (!dfa->accepts(dfa_state)->empty()){
			accepted_rules->add(dfa->accepts(dfa_state));
			if (DEBUG){
				char *label=NULL;  
				linked_set *acc=dfa->accepts(dfa_state);
				while(acc!=NULL && !acc->empty()){
					if (label==NULL){
						label=(char *)malloc(100);
						sprintf(label,"%d",acc->value());
					}else{
						char *tmp=(char *)malloc(5);
						sprintf(tmp,",%d",acc->value());
						label=strcat(label,tmp); 
						free(tmp);
					}
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		
		/* tail-NFA */
		//compute next state (the epsilon transitions had already been removed)
		FOREACH_SET(nfa_state,it){
			nfa_set *target=(*it)->get_transitions(c);
			if (target!=NULL){
				next_state->insert(target->begin(),target->end());
	            delete target;        	   
			}
		}
		delete nfa_state;
		nfa_state=next_state;
		next_state=new nfa_set();
		
		//insert border state if needed
		border_it map_it=border->find(dfa_state);
		if (map_it!=border->end()){
			//printf("%ld:: BORDER %ld !\n",i,dfa_state);
			nfa_state->insert(map_it->second->begin(), map_it->second->end());
			border_stats++;
		}
		
		//update stats
		 nfa_active[nfa_state->size()]++;
		FOREACH_SET (nfa_state,it) nfa_stats[(*it)->get_id()]++;
		
		//accepted rules computation
		linked_set *rules=new linked_set();
		FOREACH_SET (nfa_state,it) rules->add((*it)->get_accepting());		
		if (!rules->empty()){
			accepted_rules->add(rules);
			if (DEBUG){
				char *label=(char *)malloc(100);
				sprintf(label,"%d",rules->value());
				linked_set *acc=rules->succ();
				while(acc!=NULL && !acc->empty()){
					char *tmp=(char *)malloc(5);
					sprintf(tmp,",%d",acc->value());
					label=strcat(label,tmp); free(tmp);
					acc=acc->succ();
				}
				fprintf(stream,"\nrules: %s reached at character %ld \n",label,inputs);
				free(label);
			}
		}
		delete rules;
		
		//reads one more character from input stream
		inputs++;
		c=fgetc(tracefile);
	}//end while (traversal)
	
	//compute number of states in the NFA part
	unsigned tail_size=hfa->get_tail_size();	
	
	//print statistics
	//DFA
	dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	fprintf(stream,"\nhead-DFA=\n");
	fprintf(stream,"traversal statistics:: [state #, depth, # traversals, %%time]\n");
	int num=0;
	for (int j=0;j<dfa->size();j++){
		if(dfa_stats[j]!=0){
			fprintf(stream,"[%ld, %ld, %ld, %f %%]\n",j,dfa_depth[j],dfa_stats[j],(float)dfa_stats[j]*100/inputs);
			num++;
		}
	}
	fprintf(stream,"%ld out of %ld head-states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	//NFA
	fprintf(stream,"\ntail-NFA=\n");
	fprintf(stream,"traversal statistics:: [state #,#traversal, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(nfa_stats[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,nfa_stats[j],(float)nfa_stats[j]*100/inputs);
			num++;
		}
	fprintf(stream,"%ld out of %ld tail-states traversed (%f %%)\n",num,nfa->size(),(float)num*100/tail_size);
	fprintf(stream,"\ntraversal statistics:: [size of active state vector #,frequency, %%time]\n");
	num=0;
	for (int j=0;j<nfa->size();j++)
		if(nfa_active[j]!=0){
			fprintf(stream,"[%ld,%ld,%f %%]\n",j,nfa_active[j],(float)nfa_active[j]*100/inputs);
			num+=j*nfa_active[j];
		}
	fprintf(stream,"average size of active state vector %f\n",(float)num/inputs);
	fprintf(stream,"border states: [total traversal, %%time] %ld %f %%\n",border_stats,(float)border_stats*100/inputs);
	
	fprintf(stream,"\nrules matched: %ld\n",accepted_rules->size());	
	
	
	free(dfa_stats);	
	free(nfa_stats);		
	free(nfa_active);
	delete nfa_state;
	delete next_state;
	delete accepted_rules;
}

int trace::bad_traffic(DFA *dfa, state_t s){
	state_t **tx=dfa->get_state_table();
	int forward[CSIZE];
	int backward[CSIZE];
	int num_fw=0;
	int num_bw=0;
	for (int i=0;i<CSIZE;i++){
		if(dfa->get_depth()[tx[s][i]]>dfa->get_depth()[s]) forward[num_fw++]=i;
		else backward[num_bw++]=i;
	}
	//assert(num_fw+num_bw==CSIZE);
	srand(seed++);
	if (num_fw>0) return forward[randint(0,num_fw-1)];
	else return backward[randint(0,num_bw-1)];
}

int trace::avg_traffic(DFA *dfa, state_t s){
	return randint(0,CSIZE-1);
}

int trace::syn_traffic(DFA *dfa, state_t s,float p_fw){
	state_t **tx=dfa->get_state_table();
	int forward[CSIZE];
	int backward[CSIZE];
	int num_fw=0;
	int num_bw=0;
	int threshold=(int)(p_fw*10);
	for (int i=0;i<CSIZE;i++){
		if(dfa->get_depth()[tx[s][i]]>dfa->get_depth()[s]) forward[num_fw++]=i;
		else backward[num_bw++]=i;
	}
	srand(seed++);
	int selector=randint(1,10);
	if ((selector<=threshold && num_fw>0)||num_bw==0){
		num_forward_tx++;
		return forward[randint(0,num_fw-1)];
	}
	else{
		num_backward_tx++;
		return backward[randint(0,num_bw-1)];
	}
}

int trace::syn_traffic(NFA *root, nfa_set *active_set, float p_fw, bool forward){
	srand(seed++);
	int selector=randint(1,100);
	//random character
	if (selector > 100*p_fw) {
		num_random_tx++;
		return randint(0,CSIZE-1);
	} 
    //maximize moves forward
	int weight[CSIZE];
	int max_weight=0;
	int num_char=0;
	for (int c=0;c<CSIZE;c++){
		if (forward){
			NFA *next = get_next_forward(active_set,c);
			if (next!=NULL) weight[c] = next->get_depth();
			else weight[c] = 0;
		}
		else {
			nfa_set *next=get_next(active_set,c,true);
			weight[c]=next->size();
			delete next;
		}
		if (weight[c]>max_weight) max_weight=weight[c];
	}
	for (int c=0;c<CSIZE;c++) if (max_weight>0 && weight[c]==max_weight) num_char++;
	if (num_char==0){
		num_visits++;
		num_random_tx++;
		root->reset_visited(); //cleaning the visiting to allow another traversal
		return randint(0,CSIZE-1); //no forward transitions available - falling backwards
	}
	int selection=randint(1,num_char);
	int c ;
	for (c=0;c<CSIZE;c++) {
		if (weight[c]==max_weight) selection--;
		if (selection==0) break;
	}
	//character leading to a forward transition
	num_forward_tx++;
	return c;
}

nfa_set *trace::get_next(nfa_set *active_set, int c, bool forward){
	nfa_set *next_state = new nfa_set();
	FOREACH_SET(active_set,it){
		NFA *state=*it;
		nfa_set *tx = state->get_transitions(c);
		if (tx!=NULL){
			FOREACH_SET(tx,it2){
				if (!forward || (*it2)->get_depth() > state->get_depth()){
					nfa_set *closure=(*it2)->epsilon_closure();
					next_state->insert(closure->begin(),closure->end());
					delete closure;	
				}
			}
			delete tx;
		}
	}
	return next_state;
}

NFA *trace::get_next_forward(nfa_set *active_set, int c){
	NFA *result = NULL;
	FOREACH_SET(active_set,it){
		nfa_set *tx = (*it)->get_transitions(c);
		if (tx!=NULL){
			FOREACH_SET(tx,it2){
				if ((!SET_MBR(active_set,*it2) && !(*it2)->is_visited()) && (result==NULL || (*it2)->get_depth()>result->get_depth())) result=*it2;
			}
			delete tx;
		}
	}
	return result;
}

FILE *trace::generate_trace(NFA *nfa,int in_seed, float p_fw, bool forward, char *trace_name){
	
	FILE *trace=fopen(trace_name,"w");
	if (trace==NULL) fatal("trace::generate_trace(): could not open the trace file\n");
	
	//seed setting
	seed=in_seed;
	srand(seed);
	
	//some initializations...
	nfa->reset_depth();
	nfa->set_depth();
	nfa->reset_visited();
	num_forward_tx = num_random_tx = num_visits=0;
	
	nfa_set *active_set = nfa->epsilon_closure();
	unsigned inputs=0;
	unsigned int stream_size=randint(MIN_INPUTS,MAX_INPUTS);
	if (forward) FOREACH_SET(active_set,it) (*it)->visit();
	
	if (VERBOSE) printf("generating NFA trace: %s of %d bytes...\n",trace_name,stream_size);
	
	//traversal
	int c=syn_traffic(nfa,active_set,p_fw,forward);
	while(inputs<stream_size){
	
		inputs++;
		fputc(c,trace);
			
		//update active set
		nfa_set *next_state=get_next(active_set,c);
		delete active_set;
		active_set=next_state;
		if (forward) FOREACH_SET(active_set,it) (*it)->visit();
		
		//read next char
		c=syn_traffic(nfa,active_set,p_fw,forward);
		
	} //end traversal
	
	delete active_set;
	nfa->reset_visited();
	if (DEBUG) printf("trace::generate_trace(NFA): forward_tx=%d, random_tx=%d, visits=%d\n",
					  num_forward_tx,num_random_tx,num_visits);
	
	return trace;
}

FILE *trace::generate_trace(DFA *dfa,int in_seed, float p_fw, char *trace_name){
	
	FILE *trace=fopen(trace_name,"w");
	if (trace==NULL) fatal("trace::generate_trace(): could not open the trace file\n");
	
	//seed setting
	seed=in_seed;
	srand(seed);
	
	//some initializations...
	dfa->set_depth();
	unsigned inputs=0;
	unsigned int stream_size=randint(MIN_INPUTS,MAX_INPUTS);
	num_forward_tx = num_backward_tx=0;
	
	if (VERBOSE) printf("generating DFA trace: %s of %d bytes...\n",trace_name,stream_size);
	
	//traversal
	state_t state=0;
	int c=syn_traffic(dfa,state,p_fw);
	
	while(inputs<stream_size){
	
		inputs++;
		fputc(c,trace);
			
		state = dfa->get_next_state(state,c); //update active state
		c=syn_traffic(dfa,state,p_fw);    //get next character
		
	} //end traversal
	
	if (DEBUG) printf("trace::generate_trace(DFA): forward_tx=%d, backward_tx=%d\n",
						  num_forward_tx,num_backward_tx);

	return trace;
}

void trace::traverse(dfas_memory *mem, double *data){
	
	printf("trace:: traverse(dfas_memory) : using tracefile = %s\n",tracename);
	
	//tracefile
	if (tracefile!=NULL) rewind(tracefile);
	else fatal ("trace::traverse(): No tracefile\n");

    unsigned num_dfas=mem->get_num_dfas();
	DFA **dfas=mem->get_dfas();
	for (int i=0;i<num_dfas;i++) dfas[i]->set_depth();
	
	//statistics
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned max_depth=0;
	unsigned *visited=new unsigned[mem->get_num_states()];
	for (unsigned i=0;i<mem->get_num_states();i++) visited[i]=0;
	unsigned int state_traversals=0; //state traversals
	int m_size; //size of current number of memory accesses

	/* traversal */
	
	state_t *state=new state_t[num_dfas]; //current state (for each active DFA)
	for (int i=0;i<num_dfas;i++) state[i]=0;
		
	int c=fgetc(tracefile);
	while(c!=EOF){
		inputs++;
		for (int idx=0;idx<num_dfas;idx++){
			DFA *dfa=dfas[idx];
			bool fp=true;
			while (fp){
				state_traversals++;
				visited[mem->get_state_index(idx,state[idx])]++;
				//access memory
				int *m = mem->get_dfa_accesses(idx,state[idx],c,&m_size);
				mem_accesses+=m_size;
				//trace_accesses(m,m_size,mem_trace);
				
				for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
				delete [] m;
				
				//next state
				state_t next_state=NO_STATE;
				FOREACH_TXLIST(dfa->get_labeled_tx()[state[idx]],it){
					if ((*it).first==c){
						next_state=(*it).second;
						break;
					}
				}
				if (next_state==NO_STATE && mem->allow_def_tx(idx,state[idx])){ 
					state[idx]=dfa->get_default_tx()[state[idx]];
				}else{
					state[idx]=dfa->get_state_table()[state[idx]][c];
					if (dfa->get_depth()[state[idx]]>max_depth) max_depth=dfa->get_depth()[state[idx]];
					fp=false;	
				}
			}
			
			//matching
			if (!dfa->accepts(state[idx])->empty()){
				accepted_rules->add(dfa->accepts(state[idx]));  
				linked_set *acc=dfa->accepts(state[idx]);
			}
		}
		c=fgetc(tracefile);
	} //end traversal
	delete [] state;
	
	// statistics computation
	unsigned num=0;
	for (int j=0;j<mem->get_num_states();j++) if (visited[j]!=0) num++;
			
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/mem->get_num_states(); //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	delete [] visited;		
	delete accepted_rules;
}

void trace::traverse(fa_memory *mem, double *data){
		
	//tracefile
	if (tracefile!=NULL) rewind(tracefile);
	else fatal ("trace::traverse(fa_memory): No tracefile\n");

	if (mem->get_dfa()!=NULL) 
		traverse_dfa(mem, data);
	else if (mem->get_nfa()!=NULL) 
		traverse_nfa(mem, data);
	else 
		fatal("trace:: traverse - empty memory");
	
}

void trace::traverse_dfa(fa_memory *mem, double *data){
	//dfa
	DFA *dfa=mem->get_dfa();
	dfa->set_depth();
	unsigned int *dfa_depth=dfa->get_depth();
	state_t *fp=dfa->get_default_tx();
	tx_list **lab_tx=dfa->get_labeled_tx();
	state_t **tx=dfa->get_state_table();
	
	//statistics
	unsigned int *lab_stats=allocate_uint_array(dfa->size());
	unsigned int *fp_stats=allocate_uint_array(dfa->size());
	unsigned int *mem_stats=allocate_uint_array(MAX_MEM_REF);
	for (int j=0;j<dfa->size();j++){
		lab_stats[j]=0;
		fp_stats[j]=0;
	}
	for (int j=0;j<MAX_MEM_REF;j++) mem_stats[j]=0;
	lab_stats[0]++;
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned int state_traversals=0; //state traversals
	int m_size; //size of current number of memory accesses

	//traversal
	state_t state=0; 
	
	int c = fgetc(tracefile);
	
	while(c!=EOF){
		state_traversals++;
		//access memory
		int *m = mem->get_dfa_accesses(state,c,&m_size);
		mem_accesses+=m_size;
		mem_stats[m_size]++;
		for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
		delete [] m;
		
		//next state
		state_t next_state=NO_STATE;
		FOREACH_TXLIST(lab_tx[state],it){
			if ((*it).first==c){
				next_state=(*it).second;
				break;
			}
		}
		if (next_state==NO_STATE && mem->allow_def_tx(state)){ 
			state=fp[state];
			fp_stats[state]++;
		}else{
			state=tx[state][c];
			lab_stats[state]++;
			//next character
			c=fgetc(tracefile);			
			inputs++;	
		}
		
		//matching
		if (!dfa->accepts(state)->empty()){
			accepted_rules->add(dfa->accepts(state));  
			linked_set *acc=dfa->accepts(state);
			if (DEBUG){
				while(acc!=NULL && !acc->empty()){
					printf("\nrule: %d reached at character %ld \n",acc->value(),inputs);
					acc=acc->succ();
				}
			}
		}
	} //end traversal
	
	// statistics computation
	//printf("\ntraversal statistics:: [state #,depth, #lab tr, #def tr, %%lab tr, %%def tr, %%time]\n");
	int num=0;
	int more_states=0;
	int max_depth=0;
	int num_fp=0;
	for (int j=0;j<dfa->size();j++){
		num_fp+=fp_stats[j];
		if(lab_stats[j]!=0 || fp_stats[j]!=0 ){
			if (dfa_depth[j]>max_depth) max_depth=dfa_depth[j];
			//printf("[%ld, %ld, %ld, %ld, %f %%, %f %%, %f %%]\n",j,dfa_depth[j],lab_stats[j],fp_stats[j],(float)lab_stats[j]*100/state_traversals,
			//		(float)fp_stats[j]*100/state_traversals,(float)(lab_stats[j]+fp_stats[j])*100/state_traversals);		
			num++;
		}
	}
	int *depth_stats=new int[max_depth+1];
	for (int i=0;i<=max_depth;i++) depth_stats[i]=0;

#ifdef LOG	
	//for (state_t s=0;s<dfa->size();s++) depth_stats[dfa->get_depth()[s]]+=(lab_stats[s]+fp_stats[s]);
	printf("%ld out of %ld states traversed (%f %%)\n",num,dfa->size(),(float)num*100/dfa->size());
	printf("rules matched: %ld\n",accepted_rules->size());
	printf("fraction mem accesses/character %f\n",(float)mem_accesses/inputs);
	printf("state traversal - /character %d %f\n",state_traversals,(float)state_traversals/inputs);
	printf("default transitions taken: tot=%ld per input=%f\n",num_fp, (float)num_fp/inputs);
	printf("max depth reached %ld\n",max_depth);	
    /*printf("depth statistics::\n");
    for(int i=0;i<=max_depth;i++) if(depth_stats[i]!=0) printf("[%d=%f]",i,(float)depth_stats[i]*100/state_traversals);
    printf("\n");
    */ 
	printf("cache hits=%ld /hit rate=%f %%, misses=%ld /miss rate=%f %%\n",
			cache_stats[0],(float)100*cache_stats[0]/mem_accesses,cache_stats[1],(float)100*cache_stats[1]/mem_accesses);
	printf("clock cycles = %ld, /input= %f\n",(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY),
			(float)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs);			
	
#endif	
	//mem->get_cache()->debug();
			
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/dfa->size(); //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	free(fp_stats);
	free(lab_stats);
	free(mem_stats);
	delete [] depth_stats;		
	delete accepted_rules;
}


void trace::traverse_nfa(fa_memory *mem, double *data){
	//nfa
	NFA *nfa=mem->get_nfa();
	nfa->set_depth();
	
	//statistics
	int nfa_size=nfa->size();
	unsigned int *state_stats=allocate_uint_array(nfa_size);
	unsigned int *set_stats=allocate_uint_array(nfa_size);
	unsigned int *mem_stats=allocate_uint_array(MAX_MEM_REF);
	for (int j=0;j<nfa_size;j++){
		state_stats[j]=0;
		set_stats[j]=0;
	}
	for (int j=0;j<MAX_MEM_REF;j++) mem_stats[j]=0;
	
	long cache_stats[2]; cache_stats[0]=cache_stats[1]=0; //0=hit, 1=miss
	
	linked_set *accepted_rules=new linked_set();
	long mem_accesses=0; //total number of memory accesses
	unsigned int inputs=0; //total number of inputs
	unsigned int state_traversals=0; //state traversals
	int m_size =0; //size of current number of memory accesses

	//active set
	nfa_set *active_set = nfa->epsilon_closure();
 
	int c=fgetc(tracefile);
	
	while(c!=EOF){
		
		inputs++;
		//update statistics
		state_traversals+=active_set->size();
		set_stats[active_set->size()]++;
		FOREACH_SET(active_set,it) state_stats[(*it)->get_id()]++;
		 
		FOREACH_SET(active_set,it){
			
			NFA *state=*it;
			
			//memory access
			int *m = mem->get_nfa_accesses(state,c,&m_size);
			mem_accesses+=m_size;
			mem_stats[m_size]++;
			//trace_accesses(m,m_size,mem_trace);
			if (m!=NULL){
				for (int i=0;i<m_size;i++) cache_stats[mem->read(m[i])]++;
				delete [] m;
			}
			
			//matching
			if (!state->get_accepting()->empty()){
				accepted_rules->add(state->get_accepting());
				if (DEBUG){  
					linked_set *acc=state->get_accepting();
					while(acc!=NULL && !acc->empty()){
						printf("\nrule: %d reached at character %ld \n",acc->value(),inputs);
						acc=acc->succ();
					}
				}
			}
				
		}
		//update active set
		nfa_set *next_state=get_next(active_set,c);
		delete active_set;
		active_set=next_state;
		
		//read next char
		c=fgetc(tracefile);
		
	} //end traversal
	delete active_set;
	
	// statistics computation
	//printf("\ntraversal statistics:: [state #,depth, #lab tr, #def tr, %%lab tr, %%def tr, %%time]\n");
	int num=0;
	int more_states=0;
	int max_depth=0;
	
	nfa_list *queue=new nfa_list();
	nfa->traverse(queue);
	
	FOREACH_LIST(queue,it){
		NFA *state=*it;
		state_t j=state->get_id();
		if(state_stats[j]!=0){
			if (state->get_depth()>max_depth) max_depth=state->get_depth();
			//printf("[%ld, %ld, %ld, %f %%]\n",j,state->get_depth(),state_stats[j],(float)state_stats[j]*100/state_traversals);		
			num++;
		}
	}
	
	int *depth_stats=new int[max_depth+1];
	for (int i=0;i<=max_depth;i++) depth_stats[i]=0;
	FOREACH_LIST(queue,it) depth_stats[(*it)->get_depth()]+=state_stats[(*it)->get_id()];

#ifdef LOG	
	printf("p_m %f\n",p_fw);
	printf("%ld out of %ld states traversed (%f %%)\n",num,nfa_size,(float)num*100/nfa_size);
	printf("rules matched: %ld\n",accepted_rules->size());
	printf("fraction mem accesses/character %f\n",(float)mem_accesses/inputs);
	printf("state traversal - /character %d %f\n",state_traversals,(float)state_traversals/inputs);
	printf("max depth reached %ld\n",max_depth);	
	/*
    printf("depth statistics::\n");
    for(int i=0;i<=max_depth;i++) if(depth_stats[i]!=0) printf("[%d=%f%%]",i,(float)depth_stats[i]*100/state_traversals);
    printf("\n");
    */
	printf("cache hits=%ld /hit rate=%f %%, misses=%ld /miss rate=%f %%\n",
			cache_stats[0],(float)100*cache_stats[0]/mem_accesses,cache_stats[1],(float)100*cache_stats[1]/mem_accesses);
	printf("clock cycles = %ld, /input= %f\n",(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY),
			(float)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs);				
#endif
	
	//mem->get_cache()->debug();
		
	if (data!=NULL){
		data[0]=inputs; //number of inputs
		data[1]=accepted_rules->size(); //matches
		data[2]=max_depth; //max depth reached
		data[3]=(double)num*100/nfa_size; //% states traversed
		data[4]=(double)state_traversals/inputs;  //state traversal/input
		data[5]=(double)mem_accesses/inputs; //mem access/input
		data[6]=(double)100*cache_stats[0]/mem_accesses; //hit rate
		data[7]=(double)100*cache_stats[1]/mem_accesses; //miss rate
		data[8]=(double)(cache_stats[0]*CACHE_HIT_DELAY+cache_stats[1]*CACHE_MISS_DELAY)/inputs; //clock cycle/input
	}
	
	//free memory
	free(state_stats);
	free(set_stats);
	free(mem_stats);
	delete [] depth_stats;		
	delete accepted_rules;
	delete queue;
}

bool trace::pre_match(prefix_DFA *prefixDfa, match_statics* statics, FILE *stream){
    //no pre re part
    if(prefixDfa->parent_node == nullptr) return true;

    fpos_t pos_origin;
    int res;
    fgetpos(tracefile, &pos_origin);

    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;
    active_prefixDfas.push_back(prefixDfa->parent_node);
    active_states.push_back(0);

    res = fseek(tracefile, -prefixDfa->depth-1, SEEK_CUR);
    if(res != 0) {
        fsetpos(tracefile, &pos_origin);
        return true;
    }

    while(1){
        long inputs = ftell(tracefile);
        char c = fgetc(tracefile);
        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){
            //clear silent prefix_dfa
            if((*it_prefixdfa)->is_silent){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }

            //calculate CPU overhead here
            statics->active_state_num_on_character[inputs]++;
            statics->total_active_state_num++;

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
                //only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr)
                {
                    fsetpos(tracefile, &pos_origin);
                    return true;
                }
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);

        if(active_prefixDfas.empty()) break;
        if((res=fseek(tracefile, -2, SEEK_CUR)) != 0) break;
    }

    fsetpos(tracefile, &pos_origin);
    return false;
}

void trace::traverse(prefix_DFA *prefixDfa, match_statics* statics, FILE *stream) {
    if (tracefile== nullptr) fatal("trace file is NULL!");
    rewind(tracefile);

    prefixDfa->init();

    state_t state=0;
    char c=fgetc(tracefile);
    long inputs=0;
    DFA* dfa = prefixDfa->prefix_dfa;
    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;

    //while(c!=EOF){
    while(!feof(tracefile)){
        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        state=dfa->get_next_state(state,(unsigned char)c);
        if (!dfa->accepts(state)->empty()){
            statics->prefix_match_times++;
            //judge if has pre section, and try match first
            if(pre_match(prefixDfa, statics, stream)){
#ifdef MATCH_ONCE
                //only match once. here match occurs
                if(prefixDfa->next_node == nullptr) return;
#endif
                //activate post dfa (next char)
                prefix_DFA* candidate = prefixDfa->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
        }

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){
            //clear silent prefix_dfa
            if((*it_prefixdfa)->is_silent){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }

            //calculate CPU overhead here
            statics->active_state_num_on_character[inputs]++;
            statics->total_active_state_num++;

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
#ifdef MATCH_ONCE
                //only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr) return;
#endif
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);
        inputs++;
        c=fgetc(tracefile);
    }
}

bool mycomp(const pair<pair<uint32_t, uint32_t>, prefix_DFA*> &p1, const pair<pair<uint32_t, uint32_t>, prefix_DFA*> &p2){
    return p1.first.first < p2.first.first;
}

match_statics trace::traverse(list<prefix_DFA*> *prefixDfa_list, FILE *stream) {
    if (tracefile==NULL) fatal("trace file is NULL!");
    rewind(tracefile);

    match_statics statics;
    memset(&statics, 0, sizeof(statics));
    //get char number of the trace file
    int char_num = 0;
    char c;
    //for(c = getc(tracefile); c != EOF; c = getc(tracefile)){
    for(c = getc(tracefile); !feof(tracefile); c = getc(tracefile)){
        char_num += 1;
    }
    statics.char_num = char_num;
    statics.active_state_num_on_character = (unsigned int*) malloc(sizeof(unsigned int) * (char_num + 5));
    memset(statics.active_state_num_on_character, 0, sizeof(unsigned int) * (char_num+5));

    int tem_cnt = 0;
    unsigned int prefix_matching_times = 0;
    uint32_t total_states = 0;
    list<pair<pair<uint32_t, uint32_t>, prefix_DFA*>> lis_res;
    for(auto &it: *prefixDfa_list){
        //printf("traversing %d/%d prefixDfa\n", ++tem_cnt, prefixDfa_list->size());
        traverse(it, &statics, stream);
        unsigned int single_match_times = statics.prefix_match_times - prefix_matching_times;
        uint32_t single_total_states = statics.total_active_state_num - total_states;
        //printf("matching prefix times:%u\n", single_match_times);
        //printf("total states:%u\n", single_total_states);
        prefix_matching_times = statics.prefix_match_times;
        total_states = statics.total_active_state_num;

        //used to sort show
        pair<pair<uint32_t, uint32_t>, prefix_DFA*> pair = make_pair(make_pair(single_total_states, single_match_times), it);
        lis_res.push_back(pair);
    }
    lis_res.sort(mycomp);
    for(auto& it: lis_res){
        printf("complete re:%s\n", it.second->complete_re);
        printf("prefix re:%s\n", it.second->re);
        printf("single_total_states:%u, single_matching_times:%u\n", it.first.first, it.first.second);
    }

    //print statics
    unsigned int max_states_num = 0;
    for(int i=0; i<char_num; i++) {
        //statics.total_active_state_num += statics.active_state_num_on_character[i];
        max_states_num = max(max_states_num, statics.active_state_num_on_character[i]);
    }
    statics.average_active_state_num = statics.total_active_state_num * 1.0 / char_num;
    printf("#states:");
    cout << statics.total_active_state_num << endl;
    printf("#average_states:");
    cout << statics.average_active_state_num << endl;
    printf("#max_states:");
    cout << max_states_num << endl;

    statics.max_states_num = max_states_num;
    free(statics.active_state_num_on_character);
    return statics;
}

void trace::traverse_multiple(list<prefix_DFA *> *prefixDfa_list, char* file_path, FILE *stream) {
    unsigned int max_states_num = 0;
    unsigned int total_active_states_num = 0;
    unsigned int total_char_num = 0;
    unsigned int total_prefix_match_times = 0;

    struct dirent *entry;
    DIR *dp = nullptr;
    dp = opendir(file_path);
    if(dp == nullptr) fatal("failed to open dir!\n");
    while((entry = readdir(dp))){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char filename[1000];
        sprintf(filename, "%s/%s", file_path, entry->d_name);
        set_trace(filename);
        match_statics statics = traverse(prefixDfa_list);
        max_states_num = max(max_states_num, statics.max_states_num);
        total_active_states_num += statics.total_active_state_num;
        total_char_num += statics.char_num;
        total_prefix_match_times += statics.prefix_match_times;
    }
    printf("total_prefix_match_times: %u\n", total_prefix_match_times);
    printf("max_states_num: %u\n", max_states_num);
    printf("total_active_states_num: %u\n", total_active_states_num);
    printf("total_char_num: %u\n", total_char_num);
    printf("average active states num: %llf\n", total_active_states_num*1.0/total_char_num);
}

match_statics trace::traverse(list<prefix_DFA *> *prefixDfa_list, int length, char* pkt) {

    match_statics statics;
    memset(&statics, 0, sizeof(statics));

    statics.char_num = length;
    statics.active_state_num_on_character = (unsigned int*) malloc(sizeof(unsigned int) * (length + 5));
    memset(statics.active_state_num_on_character, 0, sizeof(unsigned int) * (length+5));

    int tem_cnt = 0;
    unsigned int prefix_matching_times = 0;
    uint32_t total_states = 0;
    list<pair<pair<uint32_t, uint32_t>, prefix_DFA*>> lis_res;
    for(auto &it: *prefixDfa_list){
        traverse(it, &statics, length, pkt);
        unsigned int single_match_times = statics.prefix_match_times - prefix_matching_times;
        uint32_t single_total_states = statics.total_active_state_num - total_states;
        prefix_matching_times = statics.prefix_match_times;
        total_states = statics.total_active_state_num;

        //used to sort show
        pair<pair<uint32_t, uint32_t>, prefix_DFA*> pair = make_pair(make_pair(single_total_states, single_match_times), it);
        lis_res.push_back(pair);

        it->prefix_match_times += single_match_times;
        it->cpu_overhead += single_total_states;
    }
    lis_res.sort(mycomp);
    for(auto& it: lis_res){
        if(it.first.first == 0) continue; //不输出未匹配re
        printf("complete re:%s\n", it.second->complete_re);
        printf("prefix re:%s\n", it.second->re);
        printf("single_total_states:%u, single_matching_times:%u\n", it.first.first, it.first.second);
    }

    //print statics
    unsigned int max_states_num = 0;
    for(int i=0; i<length; i++) {
        //statics.total_active_state_num += statics.active_state_num_on_character[i];
        max_states_num = max(max_states_num, statics.active_state_num_on_character[i]);
    }
    statics.average_active_state_num = statics.total_active_state_num * 1.0 / length;
    printf("#states:");
    cout << statics.total_active_state_num << endl;
    printf("#average_states:");
    cout << statics.average_active_state_num << endl;
    printf("#max_states:");
    cout << max_states_num << endl;
    printf("\n");

    statics.max_states_num = max_states_num;
    free(statics.active_state_num_on_character);
    return statics;
}

void trace::traverse(prefix_DFA *prefixDfa, match_statics *statics, int length, char *pkt) {
    prefixDfa->init();

    state_t state=0;
    int offset=0;
    char c=pkt[offset];
    DFA* dfa = prefixDfa->prefix_dfa;
    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;

    //while(c!=EOF){
    while(offset < length){
        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        state=dfa->get_next_state(state,(unsigned char)c);
        if (!dfa->accepts(state)->empty()){
            statics->prefix_match_times++;
            //debug which prefix match
            //printf("complete re:%s \t, prefix re:%s\n", prefixDfa->complete_re, prefixDfa->re);

            //judge if has pre section, and try match first
            if(pre_match(prefixDfa, statics, length, pkt, offset)){
#ifdef MATCH_ONCE
                //only match once. here match occurs
                if(prefixDfa->next_node == nullptr) return;
#endif
                //activate post dfa (next char)
                prefix_DFA* candidate = prefixDfa->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
        }

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){
#ifdef ACTIVATE_ONCE_DOTSTAR
            //clear silent prefix_dfa
            if((*it_prefixdfa)->is_silent){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
#endif

            //calculate CPU overhead here
            statics->active_state_num_on_character[offset]++;
            statics->total_active_state_num++;

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
#ifdef MATCH_ONCE
                //only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr) return;
#endif
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);
        offset++;
        c=pkt[offset];
    }
}

bool trace::pre_match(prefix_DFA *prefixDfa, match_statics *statics, int length, const char *pkt, int offset) {
    //no pre re part
    if(prefixDfa->parent_node == nullptr) return true;

    int pos_origin = offset;

    list<prefix_DFA*> active_prefixDfas;
    list<state_t> active_states;
    active_prefixDfas.push_back(prefixDfa->parent_node);
    active_states.push_back(0);

    //res = fseek(tracefile, -prefixDfa->depth-1, SEEK_CUR);
    offset -= prefixDfa->depth;

    while(offset >= 0){
        char c = pkt[offset];
        list<prefix_DFA*> add_post_dfas;
        int add_cnt = 0;

        auto it_prefixdfa = active_prefixDfas.begin();
        for(auto it_state = active_states.begin(); it_state != active_states.end(); it_state++){
#ifdef ACTIVATE_ONCE_DOTSTAR
            //clear silent prefix_dfa
            if((*it_prefixdfa)->is_silent){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
#endif

            //calculate CPU overhead here
            statics->active_state_num_on_character[offset]++;
            statics->total_active_state_num++;

            DFA* iter_dfa = (*it_prefixdfa)->prefix_dfa;
            *it_state = iter_dfa->get_next_state(*it_state, (unsigned char)c);
            if(!iter_dfa->accepts(*it_state)->empty()){
                //pre re part only match once. here match occurs
                if((*it_prefixdfa)->next_node == nullptr)
                {
                    return true;
                }
                //activate post dfa (next char)
                prefix_DFA* candidate = (*it_prefixdfa)->get_post_dfa();
                if(candidate != nullptr) {
                    add_post_dfas.push_back(candidate);
                    add_cnt++;
                }
            }
            //clear dead state & dead prefixdfa
            if(*it_state == iter_dfa->dead_state){
                it_state = active_states.erase(it_state);
                it_state--;
                it_prefixdfa = active_prefixDfas.erase(it_prefixdfa);
                it_prefixdfa--;
            }
            //increment dfa iterator
            it_prefixdfa++;
        }

        active_prefixDfas.insert(active_prefixDfas.end(), add_post_dfas.begin(), add_post_dfas.end());
        active_states.insert(active_states.end(), add_cnt, 0);

        if(active_prefixDfas.empty()) break;
        //if((res=fseek(tracefile, -2, SEEK_CUR)) != 0) break;
        offset--;
    }

    return false;
}
//bool prefix_match_times_cmp(prefix_DFA* pd1, prefix_DFA* pd2){
bool prefix_match_times_cmp(  prefix_DFA* const &pd1, prefix_DFA* const &pd2){
    return pd1->prefix_match_times > pd2->prefix_match_times;
}

bool cpu_overhead_cmp(  prefix_DFA* const &pd1, prefix_DFA* const &pd2){
    return pd1->cpu_overhead > pd2->cpu_overhead;
}

match_statics trace::traverse_pcap(list<prefix_DFA *> *prefixDfa_list, char *fname) {

    match_statics statics;
    statics.max_states_num = 0;
    statics.prefix_match_times = 0;
    statics.char_num = 0;
    statics.total_active_state_num = 0;

    int pkt_num = read_pcap(fname);
    int i = 0;
    //init prefix_match times
    for(auto &it: *prefixDfa_list){
        it->prefix_match_times = 0;
        it->cpu_overhead = 0;
    }

    for(int i=0; i<pkt_num; i++){
        printf("processing %d/%d pkt...\n", i, pkt_num);
        match_statics statics_i = traverse(prefixDfa_list, pkts[i]->len, pkts[i]->content);
        statics.max_states_num = max(statics.max_states_num, statics_i.max_states_num);
        statics.total_active_state_num += statics_i.total_active_state_num;
        statics.char_num += statics_i.char_num;
        statics.prefix_match_times += statics_i.prefix_match_times;
    }
    //debug show prefix match times of each prefix dfa
    prefixDfa_list->sort(prefix_match_times_cmp);
    for(auto &it: *prefixDfa_list){
        if(it->prefix_match_times == 0) continue;
        printf("prefix match times: %u, prefix_re:%s , complete_re: %s\n", it->prefix_match_times, it->re,  it->complete_re);
    }
    //debug show cpu over head per prefix
    prefixDfa_list->sort(cpu_overhead_cmp);
    for(auto &it: *prefixDfa_list){
        if(it->cpu_overhead == 0) continue;
        printf("cpu overhead (states): %u, prefix_re:%s , complete_re: %s\n", it->cpu_overhead, it->re,  it->complete_re);
    }

    printf("total_prefix_match_times: %u\n", statics.prefix_match_times);
    printf("max_states_num: %u\n", statics.max_states_num);
    printf("total_active_states_num: %u\n", statics.total_active_state_num);
    printf("total_char_num: %u\n", statics.char_num);
    printf("average active states num: %llf\n", statics.total_active_state_num * 1.0 / statics.char_num);

    return statics;
}

int trace::read_pcap(char * filename){
    struct pcap_pkthdr pkt_header;
    FILE* fp = fopen(filename, "r");
    if(fp == NULL){
        fprintf(stderr, "pcap file null!\n");
        exit(-1);
    }

    if(pkts){
        int i=0;
        while(pkts[i]){
            free(pkts[i]);
            i++;
        }
        free(pkts);
    }

    pkts = (Packet **) malloc(sizeof(Packet *) * MAX_PKT_NUM);
    memset(pkts, 0, sizeof(Packet *) * MAX_PKT_NUM);

    int pkt_offset = 24; //pcap文件头结构 24个字节
    int pkt_num = 0;

    while (fseek(fp, pkt_offset, SEEK_SET) == 0) //遍历数据包
    {
        //pcap_pkt_header 16 byte
        memset(&pkt_header, 0, sizeof(struct pcap_pkthdr));
        if (fread(&pkt_header, 16, 1, fp) != 1) //读pcap数据包头结构
        {
            //printf("\nread end of pcap file\n");
            break;
        }

        char* pkt_content = (char*)malloc(pkt_header.caplen);
        fread(pkt_content, pkt_header.caplen, 1, fp);

        pkt_offset += 16 + pkt_header.caplen;   //下一个数据包的偏移值
        //printf("pkt lenth:%u\n", ptk_header.caplen);

        Packet *pkt = (Packet *) malloc(sizeof(Packet));
        pkt->len = pkt_header.caplen;
        pkt->content = pkt_content;

        pkts[pkt_num] = pkt;
        pkt_num++;
        if(pkt_num >= MAX_PKT_NUM) {
            fprintf(stderr, "single pcap contain more than %d pkts!\n", MAX_PKT_NUM);
            break;
        }
    } // end while

    fclose(fp);
    return pkt_num;
}

void trace::traverse_multiple_pcap(list<prefix_DFA *> *prefixDfa_list, char *file_path) {
    unsigned int max_states_num = 0;
    unsigned int total_active_states_num = 0;
    unsigned int total_char_num = 0;
    unsigned int total_prefix_match_times = 0;

    struct dirent *entry;
    DIR *dp = nullptr;
    dp = opendir(file_path);
    if(dp == nullptr) fatal("failed to open dir!\n");
    while((entry = readdir(dp))){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char filename[1000];
        sprintf(filename, "%s/%s", file_path, entry->d_name);
        //set_trace(filename);
        match_statics statics = traverse_pcap(prefixDfa_list, filename);
        max_states_num = max(max_states_num, statics.max_states_num);
        total_active_states_num += statics.total_active_state_num;
        total_char_num += statics.char_num;
        total_prefix_match_times += statics.prefix_match_times;
    }
    printf("total_prefix_match_times: %u\n", total_prefix_match_times);
    printf("max_states_num: %u\n", max_states_num);
    printf("total_active_states_num: %u\n", total_active_states_num);
    printf("total_char_num: %u\n", total_char_num);
    printf("average active states num: %llf\n", total_active_states_num*1.0/total_char_num);
}

void trace::get_prefix_matches_core(list<prefix_DFA *> *prefixDfa_list, FILE* file, const char *pkt, int length) {

    int offset=0;
    unsigned char c = pkt[offset];
    list<DFA*> DFAs;
    list<state_t> current_states;
    for(auto &it: *prefixDfa_list){
        DFAs.push_back(it->prefix_dfa);
        current_states.push_back(0);
    }

    //while(c!=EOF){
    while(offset < length){
        auto it_state = current_states.begin();
        int id = 0; //标识第几个prefix DFA
        for(auto dfa : DFAs){
            state_t state = *it_state;
            *it_state = dfa->get_next_state(state, c);

            if(!dfa->accepts(*it_state)->empty()){
                fprintf(file, "offset:%d id:%d\n", offset, id);
            }

            it_state++;
            id++;
        }

        offset++;
        c = pkt[offset];
    }
}

void trace::get_prefix_matches(list<prefix_DFA *> *prefixDfa_list, char* pcap_fname, char *record_fname) {
    FILE* file = fopen(record_fname, "w");
    int pkt_num = read_pcap(pcap_fname);
    for(int i=0; i<pkt_num; i++){
        printf("verifying %d/%d pkt ...\n", i, pkt_num);
        fprintf(file, "verifying %d/%d pkt ...\n", i, pkt_num);
        get_prefix_matches_core(prefixDfa_list, file, pkts[i]->content, pkts[i]->len);
    }

    fclose(file);
}
