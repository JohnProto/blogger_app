/***************************************************************
 *
 * file: pss.h
 * @brief   Implementation of the "pss.h" header file for the Public Subscribe System,
 * function definitions
 *
 *
 ***************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "pss.h"


#define DPRINT(...) fprintf(stderr, __VA_ARGS__);


extern struct Group G[MG];
extern struct SubInfo *T[MG];
int value1, value2, valueP, valueM;

int Universal_Hash_Function(int x) {
    int num = ((value1*x+value2) % valueP) % valueM;
    if (num > 63) num = 63;
    else if (num < 0) num = 0;
	return num;
}


struct SubInfo* HT_Lookup(int sId) {
	int hashnumber = Universal_Hash_Function(sId);
	struct SubInfo *scroller = T[hashnumber];
	while (scroller) {
		if (scroller->sId == sId) return scroller;
		else if (scroller->sId > sId) return NULL;
		scroller = scroller->snext;
	}
	return NULL;
}



void TGPCleanup(struct TreeInfo *ctree) {
	if (!ctree) return;
	TGPCleanup(ctree->tlc);
	struct TreeInfo *rholder = ctree->trc;
	free(ctree);
	TGPCleanup(rholder);
}

void TGPInsert(int i, struct Info *info) {
	struct Subscription *subscroll = G[i].gsub;
	struct SubInfo *subscriber;
	struct TreeInfo *tree, *newinstance;
	while (subscroll) {
		subscriber = HT_Lookup(subscroll->sId);
		if (subscriber) {
			tree = subscriber->tgp[i];
			newinstance = malloc(sizeof(struct TreeInfo));
			newinstance->tId = info->iId;
			newinstance->ttm = info->itm;
			newinstance->tlc = NULL;
			newinstance->trc = NULL;
			if (!tree) {
				newinstance->tp = NULL;
				newinstance->next = NULL;
				newinstance->prev = NULL;
				subscriber->tgp[i] = newinstance;
			} else {
                while (tree->tlc) tree = tree->tlc;
                while (tree->ttm < info->itm && tree->next) tree = tree->next;
                newinstance->tp = tree;
                struct TreeInfo *tpcopy = malloc(sizeof(struct TreeInfo));
                tpcopy->tId = tree->tId;
                tpcopy->ttm = tree->ttm;
                tpcopy->tlc = NULL;
                tpcopy->trc = NULL;
                tpcopy->tp = tree;
                if (tree->next) {
                    tree->tlc = newinstance;
                    tree->trc = tpcopy;
                    tree->tId = newinstance->tId;
                    tree->ttm = newinstance->ttm;
                    newinstance->prev = tree->prev;
                    newinstance->next = tpcopy;
                    tpcopy->next = tree->next;
                    tpcopy->prev = newinstance;
                    if (tree->prev) tree->prev->next = newinstance;
                } else {
                    tree->tlc = tpcopy;
                    tree->trc = newinstance;
                    newinstance->prev = tpcopy;
                    newinstance->next = NULL;
                    tpcopy->next = newinstance;
                    tpcopy->prev = tree->prev;
                    if (tree->prev) tree->prev->next = tpcopy;
                }
                tree->prev = NULL;
                tree->next = NULL;
            }
		}
		subscroll = subscroll->snext;
	}
}


int HT_Insert(int sId, int sTM) {
	int hashnumber = Universal_Hash_Function(sId);
	struct SubInfo *scrollinsert = T[hashnumber], *prev = NULL, *newinstance = malloc(sizeof(struct SubInfo));
	if (!newinstance) return 1;
	newinstance->sId = sId;
	newinstance->stm = sTM;
	int i;
    DPRINT("Hashnumber key is: %d\n", hashnumber);
	for (i=0; i<MG; ++i) { /*By default, not subscribed to anything*/
		newinstance->tgp[i] = (struct TreeInfo *) 1;
		newinstance->sgp[i] = (struct TreeInfo *) 1;
	}
	while (scrollinsert) {
		if (sId == scrollinsert->sId) {
			free(newinstance);
			return 1;
		} else if (sId < scrollinsert->sId) {
			if (prev) {
				newinstance->snext = prev->snext;
				prev->snext = newinstance;
			} else {
				newinstance->snext = T[hashnumber];
				T[hashnumber] = newinstance;
			}
			return 0;
		}
		prev = scrollinsert;
		scrollinsert = scrollinsert->snext;
	}
	/*If the program has reached this point, subscriber will be inserted as a final element or at the beginning*/
	if (prev) prev->snext = newinstance;
	else T[hashnumber] = newinstance;
	newinstance->snext = NULL;
	return 0;
}


int HT_Delete(int sId) {
	int hashnumber = Universal_Hash_Function(sId);
	struct SubInfo *scroller = T[hashnumber], *prev = NULL;
	while (scroller) {
		if (scroller->sId == sId) {
			if (prev) prev->snext = scroller->snext;
			else T[hashnumber] = scroller->snext;
			int i;
			for (i=0; i<MG; ++i) {
				if (scroller->tgp[i] != (struct TreeInfo *) 1 && scroller->tgp[i]) {
					TGPCleanup(scroller->tgp[i]);
				}
			}
			free(scroller);
			return 0;
		} else if (scroller->sId > sId) {
			return 1;
		}
		prev = scroller;
		scroller = scroller->snext;
	}
	return 1;
}

/*void ClearPrevData(struct Info *groupinfo, int tm, int i) {
	if (!groupinfo) return;
	ClearPrevData(groupinfo->ilc, tm, i);
	struct Info *rcholder = groupinfo->irc;
	if (groupinfo->itm <= tm) {
        DPRINT("Checking info with id: %d\n", groupinfo->iId);
        if (groupinfo->ilc) {
            struct Info *tmp = rcholder;
            while (tmp->ilc) tmp = tmp->ilc;
            tmp->ilc = groupinfo->ilc;
            groupinfo->ilc->ip = tmp;
        }
		if (!groupinfo->ip) G[i].gr = rcholder;
		else groupinfo->ip->ilc = rcholder;
		if (rcholder) rcholder->ip = groupinfo->ip;
		TGPInsert(i, groupinfo);
		free(groupinfo);
	} else {
		return;
	}
	ClearPrevData(rcholder, tm, i);
}*/
void ClearPrevData(struct Info *groupinfo, int tm, int i) {
    if (!groupinfo) return;
    ClearPrevData(groupinfo->ilc, tm, i);
    struct Info *rcholder = groupinfo->irc;
    if (groupinfo->itm <= tm) {
        if (groupinfo->irc) {
            groupinfo->irc->ip = groupinfo->ip;
            if (groupinfo->ilc) {

                struct Info *scroller = groupinfo->irc;
                while (scroller->ilc) scroller = scroller->ilc;
                scroller->ilc = groupinfo->ilc;
                groupinfo->ilc->ip = scroller->ilc;
            }

            if (groupinfo->ip) {
                if (groupinfo->ip->iId > groupinfo->iId) groupinfo->ip->ilc = groupinfo->irc;
                else groupinfo->ip->irc = groupinfo->irc;
            } else {
                G[i].gr = groupinfo->irc;
            }
        } else if (groupinfo->ilc) {
            groupinfo->ilc->ip = groupinfo->ip;
            if (groupinfo->ip) {
                if (groupinfo->ip->iId > groupinfo->iId) groupinfo->ip->ilc = groupinfo->ilc;
                else groupinfo->ip->irc = groupinfo->ilc;
            } else {
                G[i].gr = groupinfo->ilc;
            }
        } else {
            if (groupinfo->ip) {
                if (groupinfo->ip->iId > groupinfo->iId) groupinfo->ip->ilc = NULL;
                else groupinfo->ip->irc = NULL;
            } else {
                G[i].gr = NULL;
            }
        }
        TGPInsert(i, groupinfo);

        free(groupinfo);
    }

    ClearPrevData(rcholder, tm, i);
}


void SL_Print() {
	DPRINT("SUBSCRIBERLIST = <");
	int i, flag=0;
	struct SubInfo *subscriber;
	for (i=0; i<MG; ++i) {
		subscriber = T[i];
		while (subscriber) {
            if (flag) DPRINT(", ");
			DPRINT("%d", subscriber->sId);
			subscriber = subscriber->snext;
			flag = 1;
		}
	}
	DPRINT(">");
}


void L_Print(int i, int printgroup) {
	if (printgroup) DPRINT("GROUPID = <%d>, ", i);
	DPRINT("SUBLIST = <");
	struct Subscription *subscriber = G[i].gsub;
	while (subscriber) {
		DPRINT("%d", subscriber->sId);
		subscriber = subscriber->snext;
		if (subscriber) DPRINT(", ");
	}
	DPRINT(">");
}


void DL_Print(struct Info *infotree, int flag) {
	if (!infotree) return;
	DL_Print(infotree->ilc, flag);
    if (flag || infotree->ilc) DPRINT(", ");
	DPRINT("%d", infotree->iId);
	DL_Print(infotree->irc, 1);
}

void TGPPrint(struct TreeInfo *t, int group) {
	struct TreeInfo *tree = t;
    if (!tree) return;
	while (tree->tlc) tree = tree->tlc;
	while (tree) {
		DPRINT("%d(<%d>)", tree->tId, group);;
		tree = tree->next;
		if (tree) DPRINT(", ");
	}
}

void SGPPrint(struct TreeInfo *t, int group) {
	DPRINT("GROUPID = <%d>, TREELIST = <", group);
	struct TreeInfo *tree = t;
	while (tree) {
		DPRINT("%d", tree->tId);
		tree = tree->next;
		if (tree) {
			t = t->next;
			DPRINT(", ");
		}
	}
	DPRINT(">, NEWSGP = <%d>", t->tId);
}


void Sub_Print(int sId) {
	DPRINT("SUBSCRIBERID = <%d>, GROUPLIST =\n", sId);
	int i;
	struct SubInfo *subscriber = HT_Lookup(sId);
	struct TreeInfo *tree;
	if (!subscriber) return;
	for (i=0; i<MG; ++i) {
		if (subscriber->tgp[i] != (struct TreeInfo *) 1) {
			DPRINT("<%d>, TREELIST = <", i);
			TGPPrint(subscriber->tgp[i], i);
			DPRINT(">");
			if (i+1 < MG) DPRINT(",\n");
		}
	}
}


/**
 * @brief Optional function to initialize data structures that
 *        need initialization
 *
 * @param m Size of the hash table.
 * @param p Prime number for the universal hash functions.
 *
 * @return 0 on success
 *         1 on failure
 */
int initialize(int m, int p){
    int i;
	for (i=0; i<MG; ++i) {
		G[i].gr = NULL;
		G[i].gsub = NULL;
		G[i].gId = i;
		T[i] = NULL;
	}
	srand(time(0));
	value1 = rand()%(((p-1) - 1 + 1) + 1);
	value2 = rand()%(((p-1) - 0 + 1) + 0);
	valueP = p;
	valueM = m;
}

/**
 * @brief Free resources
 *
 * @return 0 on success
 *         1 on failure
 */
int free_all(void){
    return EXIT_SUCCESS;
}

/**
 * @brief Insert info
 *
 * @param iTM Timestamp of arrival
 * @param iId Identifier of information
 * @param gids_arr Pointer to array containing the gids of the Event.
 * @param size_of_gids_arr Size of gids_arr including -1
 * @return 0 on success
 *          1 on failure
 */
int Insert_Info(int iTM,int iId,int* gids_arr,int size_of_gids_arr){
	int i, j, exists;
	struct Info *groupinfo, *previnfo, *newinstance;
	for (i=0; i<size_of_gids_arr-1; ++i) {
		if (gids_arr[i] < 0 || gids_arr[i] > 63) continue;
		groupinfo = G[gids_arr[i]].gr;
		previnfo = NULL;
		exists = 0;
		while (groupinfo) {
			previnfo = groupinfo;
			if (iId < groupinfo->iId) {
				groupinfo = groupinfo->ilc;
			} else if (iId > groupinfo->iId) {
				groupinfo = groupinfo->irc;
			} else {
				exists = 1;
				break;
			}
		}
		if (exists) continue;
		newinstance = malloc(sizeof(struct Info));
		if (!newinstance) return 1;
		newinstance->iId = iId;
		newinstance->itm = iTM;
		newinstance->ip = previnfo;
		newinstance->ilc = NULL;
		newinstance->irc = NULL;
		for (j=0; j<MG; ++j) newinstance->igp[i] = 0;
		for (j=0; j<size_of_gids_arr-1; ++j) {
			if (gids_arr[j] < 0 || gids_arr[j] > 63) continue;
			newinstance->igp[gids_arr[j]] = 1;
		}
		if (previnfo) {
			if (previnfo->iId > iId) previnfo->ilc = newinstance;
			else previnfo->irc = newinstance;
		} else {
			G[gids_arr[i]].gr = newinstance;
		}
		DPRINT("GROUPID = <%d>, INFOLIST = <", gids_arr[i]);
		DL_Print(G[gids_arr[i]].gr, 0);
		DPRINT(">\n");
	}
    return 0;
}
/**
 * @brief Subsriber Registration
 *
 * @param sTM Timestamp of arrival
 * @param sId Identifier of subscriber
 * @param gids_arr Pointer to array containing the gids of the Event.
 * @param size_of_gids_arr Size of gids_arr including -1
 * @return 0 on success
 *          1 on failure
 */
int Subscriber_Registration(int sTM,int sId,int* gids_arr,int size_of_gids_arr){
	if (HT_Insert(sId, sTM) == 0) {
		int i, exists;
		struct SubInfo *subscriber = HT_Lookup(sId);
		for (i=0; i<size_of_gids_arr-1; ++i) {
			if (gids_arr[i] < 0 || gids_arr[i] > 63) continue;
			/*Inserting the subscriber into the desired groups*/
            struct Subscription *newinstance, *subscroll = G[gids_arr[i]].gsub;
            exists = 0;
            while (subscroll) {
                if (subscroll->sId == sId) {
                    exists = 1;
                    break;
                }
                subscroll = subscroll->snext;
            }
            if (exists) continue;
            newinstance = malloc(sizeof(struct Subscription));
            if (!newinstance) {
                HT_Delete(sId);
                return 1;
            }
            newinstance->sId = sId;
			newinstance->snext = G[gids_arr[i]].gsub;
			G[gids_arr[i]].gsub = newinstance;
			subscriber->tgp[gids_arr[i]] = NULL;
			subscriber->sgp[gids_arr[i]] = NULL;
		}
		SL_Print();
		DPRINT("\n");
		for (i=0; i<size_of_gids_arr-1; ++i) {
			if (gids_arr[i] < 0 || gids_arr[i] > 63) continue;
			L_Print(gids_arr[i], 1);
			DPRINT("\n");
		}
		return 0;
	} else {
		return 1;
	}
}
/**
 * @brief Prune Information from server and forward it to client
 *
 * @param tm Information timestamp of arrival
 * @return 0 on success
 *          1 on failure
 */
int Prune(int tm){
    int i;
	struct Info *groupinfo;
	for (i=0; i<MG; ++i) {
		groupinfo = G[i].gr;
		ClearPrevData(groupinfo, tm, i);
		DPRINT("GROUPID = <%d>, INFOLIST=<", i);
		DL_Print(G[i].gr ,0);
		DPRINT(">, ");
		L_Print(i, 0);
		DPRINT("\n");
	}
	struct SubInfo *subscriber;
	for (i=0; i<MG; ++i) {
		subscriber = T[i];
		while (subscriber) {
			Sub_Print(subscriber->sId);
			subscriber = subscriber->snext;
			DPRINT("\n");
		}
	}
	return 0;
}
/**
 * @brief Consume Information for subscriber
 *
 * @param sId Subscriber identifier
 * @return 0 on success
 *          1 on failure
 */
int Consume(int sId){
    struct SubInfo *subscriber = HT_Lookup(sId);
	if (subscriber) {
		int i;
		for (i=0; i<MG; ++i) {
			if (subscriber->sgp[i] != (struct TreeInfo *) 1) {
				if (!subscriber->sgp[i]) {
					struct TreeInfo *tree = subscriber->tgp[i];
					if (!tree) continue;
					while (tree->tlc) tree = tree->tlc;
					subscriber->sgp[i] = tree;
				} else if (subscriber->sgp[i]->tlc) {
					subscriber->sgp[i] = subscriber->sgp[i]->tlc;
                } else {
					continue;
				}
				SGPPrint(subscriber->sgp[i], i);
				DPRINT("\n");
			}
		}
		return 0;
	}
	return 1;
}
/**
 * @brief Delete subscriber
 *
 * @param sId Subscriber identifier
 * @return 0 on success
 *          1 on failure
 */
int Delete_Subscriber(int sId){
    struct SubInfo *subscriber = HT_Lookup(sId);

    if (subscriber) {
        int i;
        struct Subscription *sub, *prevsub;
        for (i=0; i<MG; ++i) {
            if (subscriber->tgp[i] != (struct TreeInfo *) 1) {
                prevsub = NULL;
                sub = G[i].gsub;
                while (sub) {
                    if (sub->sId == sId) {
                        if (prevsub) prevsub->snext = sub->snext;
                        else G[i].gsub = sub->snext;
                        free(sub);
                        L_Print(i, 1);
                        DPRINT("\n");
                        break;
                    }
                    prevsub = sub;
                    sub = sub->snext;
                }
            }
        }
        HT_Delete(sId);
        return 0;
    }
    return 1;
}
/**
 * @brief Print Data Structures of the system
 *
 * @return 0 on success
 *          1 on failure
 */
int Print_all(void){
    int i;
    for (i=0; i<MG; ++i) {
        DPRINT("GROUPID = <%d>, INFOLIST = <", i);
        DL_Print(G[i].gr ,0);
        DPRINT(">, ");
        L_Print(i, 0);
        DPRINT("\n");
    }
    SL_Print();
    DPRINT("\n");
    int subcount = 0;
    struct SubInfo *subscriber;
    for (i=0; i<MG; ++i) {
        subscriber = T[i];
        while (subscriber) {
            Sub_Print(subscriber->sId);
            DPRINT("\n");
            ++subcount;
            subscriber = subscriber->snext;
        }
    }
    DPRINT("NO_GROUPS = <%d>, NO_SUBSCRIBERS = <%d>\n", MG, subcount);
    return 0;
}
