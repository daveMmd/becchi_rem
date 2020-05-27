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
 * File:   hybrid_fa.c
 * Author: Michela Becchi
 * Email:  mbecchi@cse.wustl.edu
 * Organization: Applied Research Laboratory
 * 
 */

#include "hybrid_fa.h"
#include "subset.h"
#include "hierarMerging.h"
#include "hfadump.h"

bool HybridFA::special(NFA *nfa){
	if (SET_MBR(non_special,nfa)) return false; 
	if (nfa->get_transitions()->size()<=MAX_TX){
		non_special->insert(nfa);
		return false;
	}	
	if (nfa->get_depth()<SPECIAL_MIN_DEPTH){
		non_special->insert(nfa);
		return false;
	}
	/*if (head->size()<MAX_HEAD_SIZE){
		non_special->insert(nfa);
		return false;
	}*/
#ifdef TAIL_DFAS
	/* if we want to ensure that each tail has only one activation, 
	 * we must exclude all those dot-star terms such that the characters
	 * excluded from the repetitions do appear in the tail */
	pair_set *tx=nfa->get_transitions();
	int_set *chars=new int_set(CSIZE);
	FOREACH_PAIRSET(tx,it) if ((*it)->second==nfa) chars->insert((*it)->first);
	if (chars->size()<=MAX_TX){
		if (DEBUG) printf ("HybridFA:: special(): NFA-state %d w/ %d forward transitions!\n",nfa->get_id(),tx->size()-chars->size());
		delete chars;
		non_special->insert(nfa);
		return false;
	}
	if (chars->size()==CSIZE){
		delete chars;
		//printf ("HybridFA:: special(): NFA-state %d is special\n",nfa->get_id());
		return true;
	}else{
		chars->negate();
		nfa_list *queue=new nfa_list();
		nfa_set  *nfas=new nfa_set();
		queue->push_back(nfa);
		nfas->insert(nfa);
		while (!queue->empty()){
			NFA *state=queue->front(); queue->pop_front();
			pair_set *tx=state->get_transitions();
			int_set *auto_chars=new int_set(CSIZE);
			FOREACH_PAIRSET(tx,it) if ((*it)->second==state) auto_chars->insert((*it)->first);
			if (auto_chars->size()!=CSIZE){ //a .* would mask the whole sub-NFA
				FOREACH_PAIRSET(tx,it){
					if (!SET_MBR(nfas,(*it)->second)){
						queue->push_back((*it)->second);
						nfas->insert((*it)->second);
					}
					if (chars->mbr((*it)->first)){
						delete chars;
						delete auto_chars;
						delete queue;
						delete nfas;
						non_special->insert(nfa);
						if (DEBUG) printf ("HybridFA:: special(): NFA-state %d w/ dot-star term but excluded chars in the tail\n",nfa->get_id());
						return false;
					}
				}
			}
			delete auto_chars;
		}
		delete chars;
		delete queue;
		delete nfas;
	}
#endif
	//printf ("HybridFA:: special(): NFA-state %d is special\n",nfa->get_id());
	return true;
}

bool HybridFA::hmspecial(NFA *nfa){
    if(nfa->get_depth() >= 10){
        return true;
    }
    if (nfa->get_transitions()->size()<=MAX_TX){
        return false;
    }
    if (nfa->get_depth()<SPECIAL_MIN_DEPTH){
        return false;
    }
    //if(root_nfa != NULL) printf ("HybridFA:: hmspecial(): NFA-state %d is special, total %d states \n", nfa->get_id(), root_nfa->size());
    return true;
}

void HybridFA::optimize_nfa_for_hfa(NFA *nfa, unsigned depth){
	
	if (DEBUG) printf("HybridFA::optimize_nfa_for_hfa(): NFA initial size : %d\n", nfa->size());
	
	nfa->set_depth();
	nfa_set  *to_correct = new nfa_set();
	nfa_list *queue     = new nfa_list();
	nfa_set  *processed = new nfa_set();
	nfa_set  *to_drop = new nfa_set();
	
	/* compute the list of states to be corrected */
	queue->push_back(nfa);
	processed->insert(nfa);
	while(!queue->empty()){
		NFA *state=queue->front(); queue->pop_front();
	    pair_set *tx=state->get_transitions();
	    int_set *chars = new int_set(CSIZE);
	    FOREACH_PAIRSET(tx,it) if ((*it)->second==state) chars->insert((*it)->first);
	    if (state->get_depth()<depth || chars->size()<=MAX_TX){
	    	FOREACH_PAIRSET(tx,it){
	    		NFA *next=(*it)->second;
	    		if (!SET_MBR(processed,next)){
	    			queue->push_back(next);
	    			processed->insert(next);
	    		}
	    	}
	    }else if (chars->size()==CSIZE){
	    	; //do nothing
	    }else{
	    	chars->negate();
	    	nfa_list *q=new nfa_list();
	    	nfa_set  *nfas=new nfa_set();
	    	q->push_back(state);
	    	nfas->insert(state);
	    	while (!q->empty()){
	    		NFA *s=q->front(); q->pop_front();
	    		pair_set *tx=s->get_transitions();
	    		int_set *auto_chars=new int_set(CSIZE);
	    		FOREACH_PAIRSET(tx,it) if ((*it)->second==s) auto_chars->insert((*it)->first);
	    		if (auto_chars->size()!=CSIZE){ //a .* would mask the whole sub-NFA
		    		FOREACH_PAIRSET(tx,it){
		    			if (!SET_MBR(nfas,(*it)->second)){
		    				q->push_back((*it)->second);
		    				nfas->insert((*it)->second);
		    			}
		    			if (chars->mbr((*it)->first)){
		    				printf ("BAD state %d on state=%d/char=%d\n",state->get_id(),s->get_id(),(*it)->first);
		    				to_correct->insert(state);
		    				q->erase(q->begin(),q->end());
		    				break;
		    			}
		    		}
	    		}
	    		delete auto_chars;
	    	}//end while
	    	delete q;
	    	delete nfas;
	    } //end else		
	    delete chars;
	}
	delete queue;
	delete processed;
	
	/* correct the states to be corrected */
	FOREACH_SET(to_correct,it){
		NFA *state = *it;               //original: remove transitions on "forbidden" characters
		NFA *copy  = (state->make_dup())->get_first(); //copy: keep only transitions on "forbidden" characters		
		
		//compute the "allowed" characters
		int_set *chars=new int_set(CSIZE);
		FOREACH_PAIRSET(state->get_transitions(),itx) if ((*itx)->second==state) chars->insert((*itx)->first);
		
		/* ============== *
		 * original state *
		 * ============== */
		
		// compute the states to keep
		nfa_set *all=new nfa_set();
		state->traverse(all); 
		nfa_set *reachable = new nfa_set();
		nfa_set *marked=new nfa_set();
		nfa_list *queue = new nfa_list();
		reachable->insert(state);
		queue->push_back(state);
		while(!queue->empty()){
			NFA *s=queue->front(); queue->pop_front();
			int_set *auto_chars=new int_set(CSIZE);
			FOREACH_PAIRSET(s->get_transitions(),itx) if ((*itx)->second==s) auto_chars->insert((*itx)->first);
			if (auto_chars->size()==CSIZE) s->traverse(reachable);
			else{
				FOREACH_PAIRSET(s->get_transitions(),itx){
					if (chars->mbr((*itx)->first) && !SET_MBR(reachable,(*itx)->second)){
						reachable->insert((*itx)->second);
						queue->push_back((*itx)->second);
					}
				}
			}
			delete auto_chars;
		}
		
		//remove from the reachable states the ones which do not lead to an accepting state
		FOREACH_SET(reachable,it){
			if (!(*it)->get_accepting()->empty()){
				queue->push_back(*it);
				marked->insert(*it);
			}
		}
		while(!queue->empty()){
			NFA *s=queue->front(); queue->pop_front();
			FOREACH_SET(reachable,it){
				FOREACH_PAIRSET((*it)->get_transitions(),itx)
					if ((*itx)->second==s && !SET_MBR(marked,*it)){
						marked->insert(*it);
						queue->push_back(*it);
					} 
			}
		}
		delete reachable;
		reachable=marked;
		
		if (!SET_MBR(reachable,state)){
			if (DEBUG) printf("optimize_nfa_for_hfa():: state=%d/depth=%d BAD tail-size=%d\n",state->get_id(),state->get_depth(),all->size());
			delete copy;
			delete chars;
			delete reachable;
			delete queue;
			delete all;
			continue;
		}else{
			FOREACH_SET(all,it) if (!SET_MBR(reachable,(*it))) to_drop->insert(*it);
			if (DEBUG) printf("optimize_nfa_for_hfa():: state=%d/depth=%d GOOD tail-size=%d\n",state->get_id(),state->get_depth(),reachable->size());
		}
		
		//update the sub-NFA from the original state
		nfa_set *processed = new nfa_set();
		queue->push_back(state);
		processed->insert(state);
		while(!queue->empty()){
			NFA *s=queue->front(); queue->pop_front();
			int_set *auto_chars=new int_set(CSIZE);
			FOREACH_PAIRSET(s->get_transitions(),itx) if ((*itx)->second==s) auto_chars->insert((*itx)->first);
			if (auto_chars->size()!=CSIZE){
				pair_set *to_remove = new pair_set();
				FOREACH_PAIRSET(s->get_transitions(),itx){
					if (!chars->mbr((*itx)->first) || !SET_MBR(reachable,(*itx)->second))  
						to_remove->insert(*itx);
					else if (!SET_MBR(processed,(*itx)->second)){
						processed->insert((*itx)->second);
						queue->push_back((*itx)->second);
					}
				}
				FOREACH_PAIRSET(to_remove,itx){
					SET_DELETE(s->get_transitions(),*itx);
					delete (*itx);
				}
				delete to_remove;
			}
			delete auto_chars;
		}
		
		//clean sets
		processed->erase(processed->begin(),processed->end());
		reachable->erase(reachable->begin(),reachable->end());
		all->erase(all->begin(),all->end());
		
		
		/* =========== * 
		 * copy state  *
		 * =========== */
		marked = new nfa_set();
		copy->traverse(all);
		
		//mark the states w/ a transition on a "forbidden character" (not masked by .*)
		queue->push_back(copy);
		processed->insert(copy);
		while(!queue->empty()){
			NFA *s=queue->front(); queue->pop_front();
			int_set *auto_chars=new int_set(CSIZE);
			FOREACH_PAIRSET(s->get_transitions(),itx) if ((*itx)->second==s) auto_chars->insert((*itx)->first);
			if (auto_chars->size()!=CSIZE){
				FOREACH_PAIRSET(s->get_transitions(),itx){
					if (!chars->mbr((*itx)->first)) marked->insert(s);
					if (!SET_MBR(processed,(*itx)->second)){
						processed->insert((*itx)->second);
						queue->push_back((*itx)->second);
					}
				}
			}
			delete auto_chars;
		}
		
		//find all the states which can reach a marked state
		reachable->insert(marked->begin(),marked->end());
		FOREACH_SET(reachable,it) queue->push_back(*it);
		while(!queue->empty()){
			NFA *s=queue->front(); queue->pop_front();
			FOREACH_SET(all,ita){
				FOREACH_PAIRSET((*ita)->get_transitions(),itx) if (((*itx)->second)==s && !SET_MBR(reachable,*ita)){
					reachable->insert(*ita);
					queue->push_back(*ita);
				}
			}
		}
		
		//add the states connected to the marked ones  
		FOREACH_SET(marked,it){
			NFA *s=*it;
			FOREACH_PAIRSET(s->get_transitions(),itx){
				if (!chars->mbr((*itx)->first)){
					((*itx)->second)->traverse(reachable);
					if ((*itx)->second==s) break;	
				}
			}
		}
		
		if (!SET_MBR(reachable,copy)) reachable->erase(reachable->begin(),reachable->end());
 		
		//delete unnecessary transitions
		FOREACH_SET(reachable,it){
			NFA *s=*it;
			pair_set *to_remove=new pair_set();
			pair_set *forbidden=new pair_set(); //sets of tx on forbidden characters
			FOREACH_PAIRSET(s->get_transitions(),itx){
				if (!chars->mbr((*itx)->first) && (*itx)->second!=s) forbidden->insert(*itx);
				if (!SET_MBR(reachable,(*itx)->second)) to_remove->insert(*itx);
			}
			FOREACH_PAIRSET(forbidden,itx){
				FOREACH_PAIRSET(s->get_transitions(),itx2)
					if (chars->mbr((*itx2)->first) && (*itx)->second==(*itx2)->second) to_remove->insert(*itx2); 
			}
			
			FOREACH_PAIRSET(to_remove,itx){
				SET_DELETE(s->get_transitions(),*itx);
				delete *itx;
			}
			delete to_remove;
			delete forbidden;
		}
		if (DEBUG) printf("optimize_nfa_for_hfa():: state=%d/depth=%d BAD tail-size=%d\n",state->get_id(),state->get_depth(),reachable->size());
		
		FOREACH_SET(all,ita){
			if (!SET_MBR(reachable,*ita)) to_drop->insert(*ita);
		}
		
		//add transitions to copy
		if (SET_MBR(reachable,copy)){
			all->erase(all->begin(),all->end());
			nfa->traverse(all);
			FOREACH_SET(all,ita){
				if (*ita!=state){
					FOREACH_PAIRSET((*ita)->get_transitions(),itx){
						if ((*itx)->second==state) (*ita)->add_transition((*itx)->first,copy);		
					}
				}
			}
		}
		
		delete chars;
		delete reachable;
		delete queue;
		delete processed;
		delete all;
		delete marked;
	}
	
	//delete unnecessary states
	nfa_set *all=new nfa_set();
	nfa->traverse(all);
	FOREACH_SET(all,it){
		FOREACH_PAIRSET((*it)->get_transitions(),itx) if (SET_MBR(to_drop,(*itx)->second)) SET_DELETE(to_drop,(*itx)->second);
	}
	FOREACH_SET(to_drop,it){
		(*it)->restrict_deletion();
		delete *it;
	}
	
	delete all;
	delete to_drop;
	delete to_correct;
	
	nfa->reset_state_id();
	if (DEBUG) printf("HybridFA::optimize_nfa_for_hfa(): NFA final size : %d\n",nfa->size());
}

	
HybridFA::HybridFA(NFA *_nfa){
	nfa=_nfa;
	head = new DFA();
	nfaList = NULL; /*add by dave*/
	non_special = new nfa_set();
	border=new map <state_t,nfa_set*>();
	printf("before remove epsilon\n");
	nfa->remove_epsilon();
    printf("finish remove epsilon\n");
	nfa->reset_state_id();
#ifdef TAIL_DFAS	
	optimize_nfa_for_hfa(nfa,SPECIAL_MIN_DEPTH);
#endif 	
	nfa->set_depth();
    printf("before build()\n");
	build();

    printf("head size(dfa): %d, border size: %d, nfa size: %d\n", head->size(), border->size(), nfa->size());
}

HybridFA::HybridFA(nfa_list *ptr_nfalist){
    //nfa=_nfa;
    //remove epsilon for each nfa && reset state id from 0 && set depth
    state_t newid = 0;
    fprintf(stderr, "before remove_epsilon && set_depth\n");
    int dit = 0;
    int size = ptr_nfalist->size();
    nfaList = new nfa_list();
    FOREACH_LIST(ptr_nfalist, it){
        //if(dit%20 == 0) fprintf(stderr, "remove_epsilon: %d/%d\n", dit, size);
        fprintf(stderr, "pattern: %s\n", (*it)->pattern);
        fprintf(stderr, "remove_epsilon: %d/%d\n", dit, size);
        dit++;
        (*it)->remove_epsilon();
        (*it)->reset_state_id();
        //(*it)->reset_state_id(newid);
        newid += (*it)->size();
        (*it)->set_depth();
        nfaList->push_back((*it));
    }
    fprintf(stderr, "\n");
    nfa = NULL;//todo, may affect trace
    //non_special = new nfa_set();
    border=new map <state_t,nfa_set*>();
    printf("before hmbuild\n");
    hmbuild(ptr_nfalist);
    printf("head size(dfa): %d, border size: %d, nfa size: %d\n", head->size(), border->size(), newid);
    //copy nfa_list
    //nfaList = ptr_nfalist;
}
	

HybridFA::~HybridFA(){
	for (border_it it = border->begin(); it!=border->end(); it++){
		delete (*it).second;
	}
	delete border;
	delete head;
	if (non_special!=NULL) delete non_special;
	if(nfaList!=NULL) delete nfaList;
}

set<state_t> *set_NFA2ids(nfa_set *fas){
	set <state_t> *ids=new set<state_t>();
	FOREACH_SET(fas,it){
		ids->insert((*it)->get_id());
	}
	return ids;
}

void HybridFA::hmbuild(nfa_list* ptr_nfalist){
    //nfa list to dfa list, gen border
    list<DFA*> *ptr_dfalist = new list<DFA *>();
    FOREACH_LIST(ptr_nfalist, it){
        DFA* dfa = nfa2dfa_withborder((*it));
        if(dfa->size() > 50){
            printf("pattern: %s , head size: %d\n", (*it)->pattern, dfa->size());
            delete dfa;
            continue;
        }
        ptr_dfalist->push_back(dfa);
    }
    head = hm_dfalist2dfa(ptr_dfalist);
    //copy, prevent delete error
    *border = *((map <state_t, nfa_set*>*) head->border);
}

DFA* HybridFA::nfa2dfa_withborder(NFA * nfa){
    // contains mapping between DFA and NFA set of states
    subset *mapping=new subset(0);
    //queue of DFA states to be processed and of the set of NFA states they correspond to
    list <state_t> *queue = new list<state_t>();
    list <nfa_set*> *mapping_queue = new list<nfa_set*>();
    //iterators used later on
    //nfa_set::iterator set_it;
    //new head state id
    state_t target_state=NO_STATE;
    //set of nfas state corresponding to target head state
    nfa_set *target=new nfa_set(); //in FA form
    set <state_t> *ids=NULL; //in id form

    /* code begins here */
    //initialize data structure starting from INITIAL STATE
    target->insert(nfa);
    ids=set_NFA2ids(target);

    DFA* dfa = new DFA(); //return DFA
    map <state_t, nfa_set*> *dfaborder =(map <state_t, nfa_set*> *) dfa->border; //

    mapping->lookup(ids, dfa, &target_state);
    delete ids;
    FOREACH_SET(target,set_it) dfa->accepts(target_state)->add((*set_it)->get_accepting());
    queue->push_back(target_state);
    mapping_queue->push_back(target);

    // process the states in the queue and adds the not yet processed DFA states
    // to it while creating them
    while (!queue->empty()){
        //dequeue an element
        state_t state=queue->front(); queue->pop_front();
        nfa_set *cl_state=mapping_queue->front(); mapping_queue->pop_front();
        //printf("DFA state %d:: NFA subset:",state); FOREACH_SET(cl_state,it) printf("%d ",(*it)->get_id()); ;printf("\n");
        // each state must be processed only once
        if(!dfa->marked(state)){
            dfa->mark(state);
            nfa_set *no_special= new nfa_set();
            FOREACH_SET(cl_state,set_it){
                NFA *_nfa=*set_it;
                if (hmspecial(_nfa)) {
                    /*if ((*border)[state]==NULL) (*border)[state]= new nfa_set();
                    (*border)[state]->insert(_nfa);*/
                    if ((*dfaborder)[state]==NULL) (*dfaborder)[state]= new nfa_set();
                    (*dfaborder)[state]->insert(_nfa);
                }
                else no_special->insert(_nfa);
            }
            //iterate other all characters and compute the next state for each of them
            for(symbol_t i=0;i<CSIZE;i++){
                target= new nfa_set();
                FOREACH_SET(no_special,set_it){
                    nfa_set *state_set=(*set_it)->get_transitions(i);
                    if (state_set!=NULL){
                        target->insert(state_set->begin(), state_set->end());
                        delete state_set;
                    }
                }

                //look whether the target set of state already corresponds to a state in the DFA
                //if the target set of states does not already correspond to a state in a DFA,
                //then add it
                ids=set_NFA2ids(target);
                bool found=mapping->lookup(ids, dfa, &target_state);
                delete ids;
                if (!found){
                    queue->push_back(target_state);
                    mapping_queue->push_back(target);
                    FOREACH_SET(target,set_it){
                        dfa->accepts(target_state)->add((*set_it)->get_accepting());
                    }
                    if (target->empty()) dfa->set_dead_state(target_state);
                }else{
                    delete target;
                }
                dfa->add_transition(state, i, target_state); // add transition to the DFA
            }//end for on character i
            delete no_special;
        }//end if state marked
        delete cl_state;
    }//end while

    //deallocate all the sets and the state_mapping data structure
    delete queue;
    delete mapping_queue;
    if (DEBUG) mapping->dump(); //dumping the NFA-DFA number of state information
    delete mapping;

    //printf("head dfa states: %d, dfa border size: %d, nfa states: %d\n", dfa->size(), dfaborder->size(), nfa->size());
    return dfa;
}
//assumes that the epsilon transitions have been removed
void HybridFA::build(){
	// contains mapping between DFA and NFA set of states
	subset *mapping=new subset(0);
	//queue of DFA states to be processed and of the set of NFA states they correspond to
	list <state_t> *queue = new list<state_t>();
	list <nfa_set*> *mapping_queue = new list<nfa_set*>();  
	//iterators used later on
	nfa_set::iterator set_it;
	//new head state id
	state_t target_state=NO_STATE;
	//set of nfas state corresponding to target head state
	nfa_set *target=new nfa_set(); //in FA form
	set <state_t> *ids=NULL; //in id form
	
	/* code begins here */
	//initialize data structure starting from INITIAL STATE
	target->insert(nfa);
	ids=set_NFA2ids(target);
	mapping->lookup(ids,head,&target_state);
	delete ids;
	FOREACH_SET(target,set_it) head->accepts(target_state)->add((*set_it)->get_accepting());
	queue->push_back(target_state);
	mapping_queue->push_back(target);
	
	// process the states in the queue and adds the not yet processed DFA states
	// to it while creating them
	while (!queue->empty()){
		//dequeue an element
		state_t state=queue->front(); queue->pop_front();
		if(state % 1000 == 0) printf("building state : %d\n", state);
		nfa_set *cl_state=mapping_queue->front(); mapping_queue->pop_front();
		//printf("DFA state %d:: NFA subset:",state); FOREACH_SET(cl_state,it) printf("%d ",(*it)->get_id()); ;printf("\n");
		// each state must be processed only once
		if(!head->marked(state)){ 
			head->mark(state);
			nfa_set *no_special= new nfa_set();
			FOREACH_SET(cl_state,set_it){
				NFA *_nfa=*set_it;
				//if (special(_nfa)) {
				 if(hmspecial(_nfa)) {
					if ((*border)[state]==NULL) (*border)[state]= new nfa_set();
					(*border)[state]->insert(_nfa);
				}
				else no_special->insert(_nfa);			
			}
			//iterate other all characters and compute the next state for each of them
			for(symbol_t i=0;i<CSIZE;i++){
				target= new nfa_set();
				FOREACH_SET(no_special,set_it){
					nfa_set *state_set=(*set_it)->get_transitions(i);
					if (state_set!=NULL){
						target->insert(state_set->begin(), state_set->end());
                    	delete state_set;
					}			   
				}
							
				//look whether the target set of state already corresponds to a state in the DFA
				//if the target set of states does not already correspond to a state in a DFA,
				//then add it
				ids=set_NFA2ids(target);
				bool found=mapping->lookup(ids,head,&target_state);
				delete ids;
				if (!found){
					queue->push_back(target_state);
					mapping_queue->push_back(target);
					FOREACH_SET(target,set_it){
						head->accepts(target_state)->add((*set_it)->get_accepting());
					}
					if (target->empty()) head->set_dead_state(target_state);
				}else{
					delete target;
				}
				head->add_transition(state,i,target_state); // add transition to the DFA
			}//end for on character i
			delete no_special;		
		}//end if state marked
		delete cl_state;
	}//end while
	
	//deallocate all the sets and the state_mapping data structure
	delete queue;
	delete mapping_queue;
	if (DEBUG) mapping->dump(); //dumping the NFA-DFA number of state information
	delete mapping;
	head->reset_marking();
	delete non_special;
	non_special=NULL;
}

/**
   * Implementation of Hopcroft's O(n log n) minimization algorithm, follows
   * description by D. Gries.
   *
   * Time: O(n log n)
   * Space: O(c n), size < 4*(5*c*n + 13*n + 3*c) byte
   */

void HybridFA::minimize() {
	
	/* transition table */
	state_t **state_array = head->get_state_table();
	
	/* accept state pointers */
	linked_set **accept_state = head->get_accepted_rules();
	
	if (VERBOSE) {
		fprintf(stderr,"Hybrid-FA:: minimize: before minimization states = %ld\n",head->size());
		fprintf(stderr,"Hybrid-FA:: minimize: before minimization border = %ld\n",border->size());
	}
	
	unsigned long i;

    // the algorithm needs the DFA to be total, so we add an error state 0,
    // and translate the rest of the states by +1
    unsigned int n = head->size()+1;

    // block information:
    // [0..n-1] stores which block a state belongs to,
    // [n..2*n-1] stores how many elements each block has
    int block[2*n]; for(i=0;i<2*n;i++) block[i]=0;    

    // implements a doubly linked mylist of states (these are the actual blocks)
    int b_forward[2*n]; for(i=0;i<2*n;i++) b_forward[i]=0;
    int b_backward[2*n]; for(i=0;i<2*n;i++) b_backward[i]=0;  

    // the last of the blocks currently in use (in [n..2*n-1])
    // (end of mylist marker, points to the last used block)
    int lastBlock = n;  // at first we start with one empty block
    int b0 = n;   // the first block    

    // the circular doubly linked mylist L of pairs (B_i, c)
    // (B_i, c) in L iff l_forward[(B_i-n)*CSIZE+c] > 0 // numeric value of block 0 = n!
    int *l_forward = allocate_int_array(n*CSIZE+1);
    for(i=0;i<n*CSIZE+1;i++) l_forward[i]=0;
    int *l_backward = allocate_int_array(n*CSIZE+1);
    for(i=0;i<n*CSIZE+1;i++) l_backward[i]=0;
        
    int anchorL = n*CSIZE; // mylist anchor

    // inverse of the transition state_array
    // if t = inv_delta[s][c] then { inv_delta_myset[t], inv_delta_myset[t+1], .. inv_delta_myset[k] }
    // is the myset of states, with inv_delta_myset[k] = -1 and inv_delta_myset[j] >= 0 for t <= j < k  
    int *inv_delta[n];
    for(i=0;i<n;i++) inv_delta[i]=allocate_int_array(CSIZE);
    int *inv_delta_myset=allocate_int_array(2*n*CSIZE); 

    // twin stores two things: 
    // twin[0]..twin[numSplit-1] is the mylist of blocks that have been split
    // twin[B_i] is the twin of block B_i
    int twin[2*n];
    int numSplit;

    // SD[B_i] is the the number of states s in B_i with delta(s,a) in B_j
    // if SD[B_i] == block[B_i], there is no need to split
    int SD[2*n]; // [only SD[n..2*n-1] is used]


    // for fixed (B_j,a), the D[0]..D[numD-1] are the inv_delta(B_j,a)
    int D[n];
    int numD;    

    // initialize inverse of transition state_array
    int lastDelta = 0;
    int inv_mylists[n]; // holds a set of mylists of states
    int inv_mylist_last[n]; // the last element
        
    int c,s;
    
    for (s=0;s<n;s++)
    	inv_mylists[s]=-1;
    
    for (c = 0; c < CSIZE; c++) {
      // clear "head" and "last element" pointers
      for (s = 0; s < n; s++) {
        inv_mylist_last[s] = -1;
        inv_delta[s][c] = -1;
      }
      
      // the error state has a transition for each character into itself
      inv_delta[0][c] = 0;
      inv_mylist_last[0] = 0;

      // accumulate states of inverse delta into mylists (inv_delta serves as head of mylist)
      for (s = 1; s < n; s++) {
        int t = state_array[s-1][c]+1; //@Michela: check this "+1"

        if (inv_mylist_last[t] == -1) { // if there are no elements in the mylist yet
          inv_delta[t][c] = s;  // mark t as first and last element
          inv_mylist_last[t] = s;
        }
        else {
          inv_mylists[inv_mylist_last[t]] = s; // link t into chain
          inv_mylist_last[t] = s; // and mark as last element
        }
      }

      // now move them to inv_delta_myset in sequential order, 
      // and update inv_delta accordingly
      for (int s = 0; s < n; s++) {
        int i = inv_delta[s][c];  inv_delta[s][c] = lastDelta;
        int j = inv_mylist_last[s];
        bool go_on = (i != -1);
        while (go_on) {
          go_on = (i != j);
          inv_delta_myset[lastDelta++] = i;
          i = inv_mylists[i];
        }
        inv_delta_myset[lastDelta++] = -1;
      }
    } // of initialize inv_delta

   
    // initialize blocks 
    
    // make b0 = {0}  where 0 = the additional error state
    b_forward[b0]  = 0;
    b_backward[b0] = 0;          
    b_forward[0]   = b0;
    b_backward[0]  = b0;
    block[0]  = b0;
    block[b0] = 1;

    for (int s = 1; s < n; s++) {
      //fprintf(stdout,"Checking state [%d]\n",(s-1));
      // search the blocks if it fits in somewhere
      // (fit in = same pushback behavior, same finalness, same lookahead behavior, same action)
      int b = b0+1; // no state can be equivalent to the error state
      bool found = false;
      while (!found && b <= lastBlock) {
        // get some state out of the current block
        int t = b_forward[b];
        //fprintf(stdout,"  picking state [%d]\n",(t-1));

        // check, if s could be equivalent with t
        found = true;// (isPushback[s-1] == isPushback[t-1]) && (isLookEnd[s-1] == isLookEnd[t-1]);
        if (found) {
          //check that accepting states are the same
          found = accept_state[s-1]->equal(accept_state[t-1]);
         	
          //check that border states are the same 
          if (found) {
          		map <state_t,set <NFA*>*>::iterator it;
          		set <NFA*> *set_s=NULL, *set_t=NULL;
          		it = border->find(s-1);
          		if (it!=border->end()) set_s=it->second;
          		it = border->find(t-1);
          		if (it!=border->end()) set_t=it->second;
          		
          		if (set_s==NULL && set_t==NULL){
          			found=true;
          		}else if (set_s==NULL || set_t==NULL){
          			found=false;
          		}else{ 
          			found=((*set_s)==(*set_t));
          		}
          } 	
         
          if (found) { // found -> add state s to block b
           // fprintf(stdout,"Found! [%d,%d] Adding to block %d\n",s-1,t-1,(b-b0));
            // update block information
            block[s] = b;
            block[b]++;
            
            // chain in the new element
            int last = b_backward[b];
            b_forward[last] = s;
            b_forward[s] = b;
            b_backward[b] = s;
            b_backward[s] = last;
          }
        }

        b++;
      }
      
      if (!found) { // fits in nowhere -> create new block
        //fprintf(stdout,"not found, lastBlock = %d\n",lastBlock);

        // update block information
        block[s] = b;
        block[b]++;
        
        // chain in the new element
        b_forward[b] = s;
        b_forward[s] = b;
        b_backward[b] = s;
        b_backward[s] = b;
        
        lastBlock++;
      }
    } // of initialize blocks
  
    //printBlocks(block,b_forward,b_backward,lastBlock);

    // initialize workmylist L
    // first, find the largest block B_max, then, all other (B_i,c) go into the mylist
    int B_max = b0;
    int B_i;
    for (B_i = b0+1; B_i <= lastBlock; B_i++)
      if (block[B_max] < block[B_i]) B_max = B_i;
    
    // L = empty
    l_forward[anchorL] = anchorL;
    l_backward[anchorL] = anchorL;

    // myset up the first mylist element
    if (B_max == b0) B_i = b0+1; else B_i = b0; // there must be at least two blocks    

    int index = (B_i-b0)*CSIZE;  // (B_i, 0)
    while (index < (B_i+1-b0)*CSIZE) {
      int last = l_backward[anchorL];
      l_forward[last]     = index;
      l_forward[index]    = anchorL;
      l_backward[index]   = last;
      l_backward[anchorL] = index;
      index++;
    }

    // now do the rest of L
    while (B_i <= lastBlock) {
      if (B_i != B_max) {
        index = (B_i-b0)*CSIZE;
        while (index < (B_i+1-b0)*CSIZE) {
          int last = l_backward[anchorL];
          l_forward[last]     = index;
          l_forward[index]    = anchorL;
          l_backward[index]   = last;
          l_backward[anchorL] = index;
          index++;
        }
      }
      B_i++;
    } 
    // end of mysetup L
    
    // start of "real" algorithm
    
    // while L not empty
    while (l_forward[anchorL] != anchorL) {
     
      // pick and delete (B_j, a) in L:

      // pick
      int B_j_a = l_forward[anchorL];      
      // delete 
      l_forward[anchorL] = l_forward[B_j_a];
      l_backward[l_forward[anchorL]] = anchorL;
      l_forward[B_j_a] = 0;
      // take B_j_a = (B_j-b0)*CSIZE+c apart into (B_j, a)
      int B_j = b0 + B_j_a / CSIZE;
      int a   = B_j_a % CSIZE;
      // determine splittings of all blocks wrt (B_j, a)
      // i.e. D = inv_delta(B_j,a)
      numD = 0;
      int s = b_forward[B_j];
      while (s != B_j) {
        // fprintf(stdout,"splitting wrt. state %d \n",s);
        int t = inv_delta[s][a];
        // fprintf(stdout,"inv_delta chunk %d\n",t);
        while (inv_delta_myset[t] != -1) {
          //fprintf(stdout,"D+= state %d\n",inv_delta_myset[t]);
          D[numD++] = inv_delta_myset[t++];
        }
        s = b_forward[s];
      }      

      // clear the twin mylist
      numSplit = 0;
    
      // clear SD and twins (only those B_i that occur in D)
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];
        SD[B_i] = -1; 
        twin[B_i] = 0;
      }
      
      // count how many states of each B_i occuring in D go with a into B_j
      // Actually we only check, if *all* t in B_i go with a into B_j.
      // In this case SD[B_i] == block[B_i] will hold.
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];

        // only count, if we haven't checked this block already
        if (SD[B_i] < 0) {
          SD[B_i] = 0;
          int t = b_forward[B_i];
          while (t != B_i && (t != 0 || block[0] == B_j) && 
                 (t == 0 || block[state_array[t-1][a]+1] == B_j)) {
            SD[B_i]++;
            t = b_forward[t];
          }
        }
      }
  
      // split each block according to D      
      for (int indexD = 0; indexD < numD; indexD++) { // for each s in D
        s = D[indexD];
        B_i = block[s];
        
        if (SD[B_i] != block[B_i]) {
          int B_k = twin[B_i];
          if (B_k == 0) { 
            // no twin for B_i yet -> generate new block B_k, make it B_i's twin            
            B_k = ++lastBlock;
            b_forward[B_k] = B_k;
            b_backward[B_k] = B_k;
            
            twin[B_i] = B_k;

            // mark B_i as split
            twin[numSplit++] = B_i;
          }
          // move s from B_i to B_k
          
          // remove s from B_i
          b_forward[b_backward[s]] = b_forward[s];
          b_backward[b_forward[s]] = b_backward[s];

          // add s to B_k
          int last = b_backward[B_k];
          b_forward[last] = s;
          b_forward[s] = B_k;
          b_backward[s] = last;
          b_backward[B_k] = s;

          block[s] = B_k;
          block[B_k]++;
          block[B_i]--;

          SD[B_i]--;  // there is now one state less in B_i that goes with a into B_j
          // fprintf(stdout,"finished move\n");
        }
      } // of block splitting

      // update L
      for (int indexTwin = 0; indexTwin < numSplit; indexTwin++) {
        B_i = twin[indexTwin];
        int B_k = twin[B_i];
        for (int c = 0; c < CSIZE; c++) {
          int B_i_c = (B_i-b0)*CSIZE+c;
          int B_k_c = (B_k-b0)*CSIZE+c;
          if (l_forward[B_i_c] > 0) {
            // (B_i,c) already in L --> put (B_k,c) in L
            int last = l_backward[anchorL];
            l_backward[anchorL] = B_k_c;
            l_forward[last] = B_k_c;
            l_backward[B_k_c] = last;
            l_forward[B_k_c] = anchorL;
          }
          else {
            // put the smaller block in L
            if (block[B_i] <= block[B_k]) {
              int last = l_backward[anchorL];
              l_backward[anchorL] = B_i_c;
              l_forward[last] = B_i_c;
              l_backward[B_i_c] = last;
              l_forward[B_i_c] = anchorL;              
            }
            else {
              int last = l_backward[anchorL];
              l_backward[anchorL] = B_k_c;
              l_forward[last] = B_k_c;
              l_backward[B_k_c] = last;
              l_forward[B_k_c] = anchorL;              
            }
          }
        }
      }
    }

       // transform the transition state_array 
    
    // trans[i] is the state j that will replace state i, i.e. 
    // states i and j are equivalent
    int trans [head->size()];
    
    // kill[i] is true iff state i is redundant and can be removed
    bool kill[head->size()];
    
    // move[i] is the amount line i has to be moved in the transition state_array
    // (because states j < i have been removed)
    int move [head->size()];
    
    // fill arrays trans[] and kill[] (in O(n))
    for (int b = b0+1; b <= lastBlock; b++) { // b0 contains the error state
      // get the state with smallest value in current block
      int s = b_forward[b];
      int min_s = s; // there are no empty blocks!
      for (; s != b; s = b_forward[s]) 
        if (min_s > s) min_s = s;
      // now fill trans[] and kill[] for this block 
      // (and translate states back to partial DFA)
      min_s--; 
      for (s = b_forward[b]-1; s != b-1; s = b_forward[s+1]-1) {
        trans[s] = min_s;
        kill[s] = s != min_s;
      }
    }
    
    // fill array move[] (in O(n))
    int amount = 0;
    for (int i = 0; i < head->size(); i++) {
      if ( kill[i] ) 
        amount++;
      else
        move[i] = amount;
    }

    int j;
    // j is the index in the new transition state_array
    // the transition state_array is transformed in place (in O(c n))
    for (i = 0, j = 0; i < head->size(); i++) {
      
      // we only copy lines that have not been removed
      map <state_t,nfa_set*>::iterator it_i,it_j;
      it_i=border->find(i);
      if ( !kill[i] ) {
        
        // translate the target states 
        for (int c = 0; c < CSIZE; c++) {
          if ( state_array[i][c] >= 0 ) {
            state_array[j][c] = trans[ state_array[i][c] ];
            state_array[j][c]-= move[ state_array[j][c] ];
          }
          else {
            state_array[j][c] = state_array[i][c];
          }
        }
        
        // translate accepting numbers and border
		if (i!=j) {
			//accepting numbers
			accept_state[j]->clear();
        	accept_state[j]->add(accept_state[i]);
        	
        	//border
        	it_j=border->find(j);
			if (it_j!=border->end()){
        		delete it_j->second;
        		border->erase(j);
        	}
        	if (it_i!=border->end()){
        		(*border)[j]=new nfa_set();
        		(*border)[j]->insert(it_i->second->begin(),it_i->second->end());
        	}
		}
        j++;
      } //end if not kill
    }//end for  
        
    //free arrays
    free(l_forward);
    free(l_backward);
    free(inv_delta_myset);
    for(int i=0;i<n;i++) free(inv_delta[i]);
    
    //free unused memory in the DFA
    for (state_t s=j;s<head->size();s++){
    	free(state_array[s]);
    	delete accept_state[s];
    }
    
    //free not used border
    for(border_it it_j=border->lower_bound(j);it_j!=border->end();it_j++)
    	delete it_j->second;
    border->erase(border->lower_bound(j),border->end());
    
    //set num states
    head->set_size(j);
    
    state_array=reallocate_state_matrix(state_array,head->size());
    accept_state=(linked_set **)reallocate_array(accept_state,head->size(),sizeof(linked_set*));   
  
	if (VERBOSE){
		 fprintf(stderr,"Hybrid-FA:: minimize: after minimization states = %ld\n",head->size());
		 fprintf(stderr,"Hybrid-FA:: minimize: after minimization border = %ld\n",border->size());
	}	
 }
 
 void HybridFA::to_dot(FILE *file, const char *title){
 	
 	/* transition table */
	state_t **state_array = head->get_state_table();
	/* accept state pointers */
	linked_set **accept_state = head->get_accepted_rules();
 	
	fprintf(file, "digraph \"%s\" {\n", title);
	for (state_t s=0;s<head->size();s++){
		if (accept_state[s]->empty()){
			if (border->find(s)==border->end())
				fprintf(file, " %ld [shape=circle];\n", s);
			else	
				fprintf(file, " %ld [shape=box];\n", s);
		}else{ 
			if (border->find(s)==border->end())
				fprintf(file, " %ld [shape=doublecircle,label=\"%ld/",s,s);
			else	
				fprintf(file, " %ld [shape=box,label=\"%ld/",s,s);
			linked_set *ls=	accept_state[s];
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
	int *mark=allocate_int_array(CSIZE);
	char *label=NULL;
	char *temp=(char *)malloc(100);
	state_t target=NO_STATE;
	for (state_t s=0;s<head->size();s++){
		for(int i=0;i<CSIZE;i++) mark[i]=0;
		for (int c=0;c<CSIZE;c++){
			if (!mark[c]){
				mark[c]=1;
				if (state_array[s][c]!=0){
					target=state_array[s][c];
					//fprintf(stdout,"state_array[%ld][%d]=%ld\n",s,c,target);
					label=(char *)malloc(100);
					if ((c>='a' && c<='z') || (c>='A' && c<='Z')) sprintf(label,"%c",c);
					else sprintf(label,"%d",c);
					bool range=true;
					int begin_range=c;
					for(int d=c+1;d<CSIZE;d++){
						if (state_array[s][d]==target){
							mark[d]=1;
							if (!range){
								if ((d>='a' && d<='z') || (d>='A' && d<='Z')) sprintf(temp,"%c",d);
								else sprintf(temp,"%d",d);
								label=strcat(label,",");
								label=strcat(label,temp);
								begin_range=d;
								range=1;
							}
						}
						if (range && (state_array[s][d]!=target || d==CSIZE-1)){
							range=false;
							if(begin_range!=d-1){
								if (state_array[s][d]!=target)
									if ((d-1>='a' && d-1<='z') || (d-1>='A' && d-1<='Z')) sprintf(temp,"%c",d-1);
									else sprintf(temp,"%d",d-1);
								else
									if ((d>='a' && d<='z') || (d>='A' && d<='Z')) sprintf(temp,"%c",d);
									else sprintf(temp,"%d",d);
								label=strcat(label,"-");
								label=strcat(label,temp);
							}
						}	
					}	
				}
			}
			if (label!=NULL) {
				fprintf(file, "%ld -> %ld [label=\"%s\"];\n", s,target,label);	
				free(label); 
				label=NULL;
			}
		}
	}
	nfa_set *border_state=new nfa_set();
	for(border_it it=border->begin();it!=border->end();it++){
		border_state->insert(it->second->begin(),it->second->end());
	}
	FOREACH_SET(border_state,it){
		(*it)->to_dot(file,true);
		(*it)->reset_marking();
	}
	for(border_it it=border->begin();it!=border->end();it++){
		state_t dfa_id=it->first;
		nfa_set *nfas=it->second;
		FOREACH_SET(nfas,it2){
			fprintf(file, "%ld -> N%ld [color=\"cyan\"];\n", dfa_id,(*it2)->get_id());
		}	
	}
	delete border_state;
	free(temp);
	free(mark);
	fprintf(file,"}");
}

 unsigned HybridFA::get_tail_size(){
 	int result=0;
 	nfa_set *border_state=new nfa_set();
 	nfa_set *tail=new nfa_set();
 	for(border_it it=border->begin();it!=border->end();it++){
 		border_state->insert(it->second->begin(),it->second->end());
 	}
 	FOREACH_SET(border_state,it) (*it)->traverse(tail);
 	FOREACH_SET(border_state,it) (*it)->reset_marking();
 	result=tail->size();
 	delete tail;
 	delete border_state;
 	return result;
 }
 
 unsigned HybridFA::get_num_tails(){
  	int result=0;
  	nfa_set *tails=new nfa_set();
  	for(border_it it=border->begin();it!=border->end();it++){
  		tails->insert(it->second->begin(),it->second->end());
  	}
  	result=tails->size();
  	delete tails;
  	return result;
  }

  int HybridFA::rearrange_nfaids(){
      //NFA state id rearrange(from 0 to n-1)
      nfa_list *temp_nfaList = new nfa_list();
      int id = 0;
      FOREACH_LIST(nfaList, it){
          nfa_list nfas;
          (*it)->traverse(&nfas);
          nfa_list *ptr_nfas = &nfas;
          FOREACH_LIST(ptr_nfas, it2){
              (*it2)->id = (id++);
              temp_nfaList->push_back(*it2);
          }
      }

      delete nfaList;
      nfaList = temp_nfaList;
      return id;
    }

void* HybridFA::dumphead(int &accept_node_offset, int &nfaset_off){
    dfa_mem_block* dfa_mem = (dfa_mem_block*) malloc(sizeof(dfa_mem_block) * head->size());

    state_t** stt = head->get_state_table();
    for(int state=0; state < head->size(); state++){
        memcpy(dfa_mem[state].nextstates, stt[state], sizeof(state_t)*ALPHABET_SIZE);
        assert(head->accepts(state)->size() < 16); //maximum accept rules supported by one node
        dfa_mem[state].stateaccepts.accept_num = head->accepts(state)->size();
        dfa_mem[state].stateaccepts.accept_offset = accept_node_offset;
        accept_node_offset += head->accepts(state)->size();
        //check if the state is in border
        map<state_t, nfa_set *>::iterator mapit = border->find(state);
        if(mapit == border->end()){
            dfa_mem[state].isborder=VALUE_NULL;
        }
        else{
            dfa_mem[state].offset_to_nfaset = nfaset_off;
            nfaset_off += mapit->second->size();
        }
    }

    return dfa_mem;
}

void* HybridFA::dumpnfa(int nfasize, int &accept_node_offset, int &nfaset_offset){
    //todo
    nfa_mem_block* nfa_mem = (nfa_mem_block *) malloc(sizeof(nfa_mem_block) * nfasize);
    FOREACH_LIST(nfaList, it){
        state_t nfa_id = (*it) ->id;
        for(symbol_t c=0; c<ALPHABET_SIZE; c++){
            nfa_set *nextstates =(*it)->get_transitions(c);
            if(nextstates == NULL) nfa_mem[nfa_id].nextstates_offset[c] = VALUE_NULL;
            else{
                nfa_mem[nfa_id].nextstates_offset[c] = nfaset_offset;
                nfaset_offset += nextstates->size();
                nfaset_offset++; //one slot indicating tail
            }
            //free
            if(nextstates != NULL) delete nextstates;
        }

        linked_set* accept_rules = (*it)->get_accepting();
        assert(accept_rules->size() < 16);
        nfa_mem[nfa_id].stateaccepts.accept_num = accept_rules->size();
        accept_node_offset += accept_rules->size();
    }
    return nfa_mem;
}

void* HybridFA::dumpnfaset(int nfaset_offset){
    //todo
    state_t *nfaset_mem = (state_t *) malloc(sizeof(state_t) * nfaset_offset);

    nfaset_offset = 0;
    //nfa nfaset
    FOREACH_LIST(nfaList, it) {
        for(symbol_t c=0; c<ALPHABET_SIZE; c++) {
            nfa_set *nextstates = (*it)->get_transitions(c);
            if(nextstates == NULL) continue;
            FOREACH_SET(nextstates, it2){
                nfaset_mem[nfaset_offset++] = (*it2)->id;
            }
            nfaset_mem[nfaset_offset++] = VALUE_END;
            //free
            if(nextstates != NULL) delete nextstates;
        }
    }

    //border nfaset
    for(int state=0; state < head->size(); state++){

        //check if the state is in border
        map<state_t, nfa_set *>::iterator mapit = border->find(state);
        if(mapit == border->end()) continue;
        else{
            nfa_set* nfaSet = mapit->second;
            FOREACH_SET(nfaSet, it){
                nfaset_mem[nfaset_offset++] = (*it)->id;
            }
            nfaset_mem[nfaset_offset++] = VALUE_END;
        }
    }

    return nfaset_mem;
}

void* HybridFA::dumpaccept(int accept_node_offset){
    //todo
    node_set_app_id_and_aging* accept_node_mem = (node_set_app_id_and_aging*) malloc(sizeof(node_set_app_id_and_aging) * accept_node_offset);
    accept_node_offset = 0;

    //NFA accept
    FOREACH_LIST(nfaList, it){
        linked_set* accept_rules = (*it)->get_accepting();
        while(accept_rules != NULL && !accept_rules->empty()){
            accept_node_mem[accept_node_offset++].app_id = accept_rules->value();
            accept_rules = accept_rules->succ();
        }
    }

    //DFA accept
    for(int state=0; state < head->size(); state++){
        linked_set* accept_rules = head->accepts(state);
        while(accept_rules != NULL && !accept_rules->empty()){
            accept_node_mem[accept_node_offset++].app_id = accept_rules->value();
            accept_rules = accept_rules->succ();
        }
    }

    return accept_node_mem;
}
#if 1
  void HybridFA::dumpmem(char* fname){
      //NFA state id rearrange(from 0 to n-1)
    int nfasize =  rearrange_nfaids();

    FILE* fp = fopen(fname, "w");
    int nfaset_off = 0;//record how many slots needed by nfa_nfaset
    int accept_node_offset = 0;//record how many accept nodes

    //dump NFA state && accept node slots && caclulate nfaset slots
    void *n_nfaset_mem;
    void* nfa_mem = dumpnfa(nfasize, accept_node_offset, nfaset_off);
    fprintf(fp, "nfa_mem %d\n", nfasize);
    fwrite(nfa_mem, sizeof(nfa_mem_block), nfasize, fp);

    //dump DFA state && calculate accept node slots && calculate border nfaset slots
    int dfa_size = head->size();
    void* dfa_mem = dumphead(accept_node_offset, nfaset_off);
    fprintf(fp, "dfa_mem %d\n", dfa_size);
    fwrite(dfa_mem, sizeof(dfa_mem_block), dfa_size, fp);

    //dump nfaset (including nfa state --> nfaset && border state --> nfaset)
    void* nfaset_mem = dumpnfaset(nfaset_off);
    fprintf(fp, "nfaset_mem %d\n", nfaset_off);
    fwrite(nfaset_mem, sizeof(state_t), nfaset_off, fp);

    //dump accept node
    void* acceptnode_mem = dumpaccept(accept_node_offset);
    fprintf(fp, "node_mem %d\n", accept_node_offset);
    fwrite(acceptnode_mem, sizeof(node_set_app_id_and_aging), accept_node_offset, fp);

    //free
    free(dfa_mem);
    free(nfa_mem);
    free(nfaset_mem);
    free(acceptnode_mem);
    fclose(fp);
}
#else
void HybridFA::dumpmem(char* fname){
    //rearrange NFA ids
    rearrange_nfaids();

    //dump accept info
    int accept_node_size = 0;

    //dump head dfa (dfa_mem)
    state_t dfastate = 0;
    dfa_mem_block* dfa_mem = (dfa_mem_block*) malloc(sizeof(dfa_mem_block) * head->size());
    memset(dfa_mem, 0, sizeof(dfa_mem_block) * head->size());
    state_t** stt = head->get_state_table();
    for(; dfastate < head->size(); dfastate++){
        memcpy(dfa_mem[dfastate].nextstates, stt[dfastate], ALPHABET_SIZE * sizeof(state_t));
        //do accept later
        linked_set *accepted_rules = head->accepts(dfastate);
        accept_node_size += accepted_rules->size();
        //accept_node_size += head->accepts(dfastate)->size();
    }

    //dump tail nfa (nfa_mem)
    int nfasize = 0;
    FOREACH_LIST(nfaList, it){
        printf("nfa pattern: %s\n", (*it)->pattern);
        nfasize += (*it)->size();
    }
    nfa_mem_block* nfa_mem = (nfa_mem_block*) malloc(sizeof(nfa_mem) * nfasize);
    /*nfa id 是否正确， 待验证 todo*/
    //int total_id = 0;
    int nfaset_offset = 0; //nfastate 指向的nfaset 偏移
    FOREACH_LIST(nfaList, it){
        //NFA* nfa = (*it);
        //debug print
        printf("nfa pattern: %s\n", (*it)->pattern);
        nfa_list *ptr_nfas = new nfa_list();
        (*it)->traverse(ptr_nfas);
        //nfa_list *ptr_nfas=&nfas;
        FOREACH_LIST(ptr_nfas, it2){
            accept_node_size += (*it2)->get_accepting()->size();
            int id = (*it2)->id;
            for(int c=0; c<ALPHABET_SIZE; c++){
                nfa_set* nfaSet = (*it2)->get_transitions(c);
                if(nfaSet == NULL) {
                    nfa_mem[id].nextstates_offset[c] = VALUE_NULL;
                    continue;
                }
                nfa_mem[id].nextstates_offset[c] = nfaset_offset;
                nfaset_offset += nfaSet->size();
                nfaset_offset += 1;//one slot indicating the end of one set
                delete nfaSet;
            }
        }
        delete ptr_nfas;
        /*
        nfa->get_id2state();
        for(int i=0; i<(nfa->size()); i++){
            int id = total_id + i; //id in nfa_mem
            NFA* nfastate = nfa->id2state[i];
            nfastate->glb_id = id;
            for(int c=0; c<ALPHABET_SIZE; c++){
                nfa_set* nfaSet = nfastate->get_transitions(c);
                if(nfaSet == NULL) {
                    nfa_mem[id].nextstates_offset[c] = VALUE_NULL;
                    continue;
                }
                nfa_mem[id].nextstates_offset[c] = nfaset_offset;
                nfaset_offset += nfaSet->size();
                nfaset_offset += 1;//one slot indicating the end of one set
            }
            accept_node_size += nfastate->get_accepting()->size();
        }
        total_id += nfa->size();*/
    }

    //dump border
    map<state_t, nfa_set*>::iterator map_it = border->begin();
    while(map_it != border->end()){
        dfa_mem[map_it->first].offset_to_nfaset = nfaset_offset;
        nfaset_offset += map_it->second->size();
        nfaset_offset += 1;//one slot indicating the end of one set
        map_it++;
    }

    /*second round: fullfil nfaset(nfaset_mem) && accept(node_mem)*/

    unsigned int* nfaset_mem = (unsigned int*) malloc(sizeof(state_t) * nfaset_offset);
    node_set_app_id_and_aging* node_mem = (node_set_app_id_and_aging*) malloc(sizeof(node_set_app_id_and_aging) * accept_node_size);

    //dfa accept
    dfastate = 0;
    accept_node_size = 0;
    for(; dfastate < head->size(); dfastate++){
        linked_set *accepted_rules = head->accepts(dfastate);
        if(accepted_rules->size() == 0) continue;
        if(accepted_rules->size() > 15)
        {
            printf("accepted rule num > 15 in one state!\n");
            exit(-1);
        }
        dfa_mem[dfastate].stateaccepts.accept_num = accepted_rules->size();
        dfa_mem[dfastate].stateaccepts.accept_offset = accept_node_size;
        int acc_size = accepted_rules->size();
        for(int i=accept_node_size; i<accept_node_size+acc_size; i++){
            node_mem[i].app_id = accepted_rules->value();
            accepted_rules = accepted_rules->succ();
        }
        accept_node_size += acc_size;
    }

    //nfa nfaset && accept
    nfaset_offset = 0; //nfastate 指向的nfaset 偏移
    FOREACH_LIST(nfaList, it){
        NFA* nfa = (*it);
        nfa_list nfas;
        (*it)->traverse(&nfas);
        nfa_list *ptr_nfas=&nfas;
        FOREACH_LIST(ptr_nfas, it2) {
            int id = (*it2)->id;
            for(int c=0; c<ALPHABET_SIZE; c++){
                nfa_set* nfaSet = (*it2)->get_transitions(c);
                if(nfaSet == NULL) continue;
                FOREACH_SET(nfaSet, it3){
                    nfaset_mem[nfaset_offset++] = (*it3)->id;
                }
                nfaset_mem[nfaset_offset++] = VALUE_END;
            }
            linked_set* accepting_rules = (*it2)->get_accepting();
            int acc_size = accepting_rules->size();
            if(acc_size == 0) continue;
            if(acc_size > 15) {
                printf("accepted rule num > 15 in one state!\n");
                exit(-1);
            }
            nfa_mem[id].stateaccepts.accept_num = acc_size;
            nfa_mem[id].stateaccepts.accept_offset = accept_node_size;
            for(int i=accept_node_size; i<accept_node_size+acc_size; i++){
                node_mem[i].app_id = accepting_rules->value();
                accepting_rules = accepting_rules->succ();
            }
            accept_node_size += acc_size;
        }
        /*
        for(int i=0; i<(nfa->size()); i++){
            int id = total_id + i; //id in nfa_mem
            NFA* nfastate = nfa->id2state[i];
            for(int c=0; c<ALPHABET_SIZE; c++){
                nfa_set* nfaSet = nfastate->get_transitions(c);
                if(nfaSet == NULL) continue;
                //nfa_mem[id].nextstates_offset[c] = nfaset_offset;
                FOREACH_SET(nfaSet, it){
                    nfaset_mem[nfaset_offset++] = (*it)->id;
                }
                nfaset_mem[nfaset_offset++] = VALUE_END;
            }
            linked_set* accepting_rules = nfastate->get_accepting();
            int acc_size = accepting_rules->size();
            if(acc_size == 0) continue;
            if(acc_size > 15) {
                printf("accepted rule num > 15 in one state!\n");
                exit(-1);
            }
            nfa_mem[id].stateaccepts.accept_num = acc_size;
            nfa_mem[id].stateaccepts.accept_offset = accept_node_size;
            for(int i=accept_node_size; i<accept_node_size+acc_size; i++){
                node_mem[i].app_id = accepting_rules->value();
                accepting_rules = accepting_rules->succ();
            }
            accept_node_size += acc_size;
        }
        total_id += nfa->size();*/
    }
    //border nfaset
    map_it = border->begin();
    while(map_it != border->end()){
        nfa_set* nfaSet = map_it->second;
        FOREACH_SET(nfaSet, it){
            nfaset_mem[nfaset_offset++] = (*it)->id;
        }
        nfaset_mem[nfaset_offset++] = VALUE_END;
        map_it++;
    }

    //dump to file
    FILE* fdump = fopen(fname, "w");
    //fprintf(fdump, "dfa_mem %d\n", head->size());
    int headsize = head->size();
    fwrite(&headsize, sizeof(int), 1, fdump);
    fwrite(dfa_mem, sizeof(dfa_mem_block), head->size(), fdump);
    fprintf(fdump, "nfa_mem %d\n", nfasize);
    fwrite(nfa_mem, sizeof(nfa_mem_block), nfasize, fdump);
    fprintf(fdump, "nfaset_mem %d\n", nfaset_offset);
    fwrite(nfaset_mem, sizeof(unsigned int), nfaset_offset, fdump);
    fprintf(fdump, "node_mem %d\n", accept_node_size);
    fwrite(node_mem, sizeof(node_set_app_id_and_aging), accept_node_size, fdump);
    fclose(fdump);

    //free
    free(dfa_mem);
    free(nfa_mem);
    free(nfaset_mem);
    free(node_mem);
}
#endif
