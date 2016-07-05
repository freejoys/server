#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "minheap.h"



struct node {
	struct list_node list_node;
	struct element elt;
	struct node   *parent;
	int 		   x;
	int 		   y;
	int			   block;
	float 		   G;
	float 		   H;
	float 		   F;
};

struct map {
	int 		 scene;
	int 		 width;
	int 		 heigh;
	struct node *node;
	char 		*data;
};

struct pathfinder {
	int 			 size; 
	struct map 		*map_mgr;
	struct minheap 	*open_list;
	struct list   close_list;
	struct list 	 neighbors;
};

int direction[8][2] = {
	{-1, 0},//上
	{ 1, 0},//下
	{ 0,-1},//左
	{ 0, 1},//右
	{-1,-1},//左上
	{-1, 1},//右上
	{ 1,-1},//左下
	{ 1, 1},//右下
};


struct node *
find_node(struct map * mp,int x,int y) {
	if (x < 0 || x >= mp->width || y < 0 || y >= mp->heigh) {
		return NULL;
	}
	return &mp->node[x*mp->heigh + y];
}

void
clear_neighbors(struct pathfinder * pf) {
	while(!list_empty(&pf->neighbors))
		list_pop(&pf->neighbors);
}

struct list *
find_neighbors(struct pathfinder * pf,int index,struct node * node) {
	clear_neighbors(pf);
	int i;
	for(i = 0;i < 8;i++) {
		int x = node->x + direction[i][0];
		int y = node->y + direction[i][1];
		struct node * tmp = find_node(&pf->map_mgr[index],x,y);
		if (tmp) {
			if (tmp->list_node.pre || tmp->list_node.next)
				continue;
			if (tmp->block == 1)
				list_push(&pf->neighbors,(struct list_node*)tmp);
		}
	}
	if (list_empty(&pf->neighbors)) 
		return NULL;
	else
		return &pf->neighbors;
}

static inline float
neighbor_cost(struct node * from,struct node * to) {
	int dx = from->x - to->x;
	int dy = from->y - to->y;
	int i;
	for(i=0;i<8;++i) {
		if (direction[i][0] == dx && direction[i][1] == dy)
			break;
	}
	if (i<4) {
		return 50.0f;
	} else if (i<8) {
		return 60.0f;
	} else {
		assert(0);
		return 0.0f;
	}
}

static inline float 
goal_cost(struct node * from,struct node * to) {
	int dx = abs(from->x - to->x);
	int dy = abs(from->y - to->y);
	return (dx * 50.0f) + (dy * 50.0f);
}

void
reset(struct pathfinder * pf) {
	struct node * n = NULL;
	while((n=(struct node*)list_pop(&pf->close_list))) {
		n->G = n->H = n->F = 0;
	}
	minheap_clear(pf->open_list);
}

static int 
less(struct element * left,struct element * right) {
	struct node *l = (struct node*)((int8_t*)left-sizeof(struct list_node));
	struct node *r = (struct node*)((int8_t*)right-sizeof(struct list_node));
	return l->F < r->F;
}

static void
clear_node(struct node *n) {
	n->parent = NULL;
	n->F = n->G = n->H = 0;
	n->elt.index = 0;
}

static void
make_table(lua_State *L,int *index,int x,int y) {
	lua_pushinteger(L, x);
	lua_rawseti(L, -2, (*index)++);
	lua_pushinteger(L, y);
	lua_rawseti(L, -2, (*index)++);
}

void
make_smooth(lua_State *L,struct node *current,struct node *from) {
	int index = 1;
	make_table(L,&index,current->x,current->y);

	struct node * parent = current->parent;
	assert(parent != NULL);
	int dx0 = current->x - parent->x;
	int dy0 = current->y - parent->y;

	current = parent;
	while(current) {
		list_remove((struct list_node*)current);
		if (current != from) {
			parent = current->parent;
			if (parent != NULL) {
				int dx1 = current->x - parent->x;
				int dy1 = current->y - parent->y;
				if (dx0 != dx1 || dy0 != dy1) {
					make_table(L,&index,current->x,current->y);
					dx0 = dx1;
					dy0 = dy1;
				}
			} else {
				make_table(L,&index,current->x,current->y);
				clear_node(current);
				break;
			}
	
		} else {
			make_table(L,&index,current->x,current->y);
			clear_node(current);
			break;
		}
		struct node *tmp = current;
		current = current->parent;
		clear_node(tmp);
	}
}

int
find_path(lua_State *L,struct pathfinder * pf,int index,int x0,int y0,int x1,int y1) {
	struct node * from = find_node(&pf->map_mgr[index],x0,y0);
	struct node * to = find_node(&pf->map_mgr[index],x1,y1);
	if (!from || !to || from == to || to->block == 0)
		return 0;

	minheap_push(pf->open_list,&from->elt);

	struct node * current = NULL;

	for(;;) {
		struct element * elt = minheap_pop(pf->open_list);
		if (!elt) {
			reset(pf);
			return 0;
		}
		current = (struct node*)((int8_t*)elt - sizeof(struct list_node));
		if (current == to) {
			make_smooth(L,current,from);
			reset(pf);
			return 1;
		}

		list_push(&pf->close_list,(struct list_node *)current);
		struct list * neighbors = find_neighbors(pf,index,current);
		if (neighbors) {
			struct node * n;
			while((n=(struct node*)list_pop(neighbors))) {
				if (n->elt.index) {
					int nG = current->G + neighbor_cost(current,n);
					if (nG < n->G) {
						n->G = nG;
						n->F = n->G + n->H;
						n->parent = current;
						minheap_change(pf->open_list,&n->elt);
					}
				} else {
					n->parent = current;
					n->G = current->G + neighbor_cost(current,n);
					n->H = goal_cost(n,to);
					n->F = n->G + n->H;
					minheap_push(pf->open_list,&n->elt);
				}
			}
		}
	}
}

int
_path_create(lua_State *L) {
	int size = lua_tointeger(L,1);
	struct pathfinder *pf = malloc(sizeof(*pf));
	memset(pf,0,sizeof(*pf));

	pf->size = size;
	pf->map_mgr = malloc(sizeof(struct map) * pf->size);
	memset(pf->map_mgr,0,sizeof(struct map) * pf->size);

	pf->open_list = minheap_new(50 * 50,less);
	list_init(&pf->close_list);
	list_init(&pf->neighbors);
	lua_pushlightuserdata(L, pf);
	return 1;
}

int
_path_release(lua_State *L) {
	struct pathfinder * pf = lua_touserdata(L, 1);
	int i;
	for(i=0;i<pf->size;i++) {
		struct map * mp = &pf->map_mgr[i];
		free(mp->node);
		free(mp->data);
	}
	free(pf->map_mgr);
	free(pf);
	return 0;
}

int
_path_init(lua_State *L) {
	struct pathfinder *pf = lua_touserdata(L, 1);
	int index = lua_tointeger(L,2);
	int scene = lua_tointeger(L,3);
	assert(index < pf->size);

	size_t size;
	const char *file = luaL_checklstring(L, 4, &size);

	struct map *mp = &pf->map_mgr[index];
	mp->scene = scene;

	FILE *stream = fopen(file,"rb+");
	assert(stream != NULL);

	fread(&mp->width,sizeof(int),1,stream);
	fseek(stream,sizeof(int),SEEK_SET);
	fread(&mp->heigh,sizeof(int),1,stream);
	fseek(stream,sizeof(int) * 2,SEEK_SET);

	mp->node = malloc(mp->width * mp->heigh * sizeof(struct node));
	memset(mp->node,0,mp->width * mp->heigh * sizeof(struct node));

	mp->data = malloc(mp->width * mp->heigh);
	memset(mp->data,0,mp->width * mp->heigh);
	fread(mp->data,mp->width * mp->heigh,1,stream);

	int i = 0;
	int j = 0;
	for( ; i <  mp->width; ++i) {
		for(j = 0; j < mp->heigh;++j) {		
			int index = i*mp->heigh+j;
			struct node *tmp = &mp->node[index];
			tmp->x = i;
			tmp->y = j;
			tmp->block = mp->data[index];
		}
	}	

	fclose(stream);

	lua_pushinteger(L,mp->width);
	lua_pushinteger(L,mp->heigh);
	return 2;
}

int
_path_find(lua_State *L) {
	struct pathfinder *pf = lua_touserdata(L, 1);
	int index = lua_tointeger(L,2);
	int x0 = lua_tointeger(L,3);
	int y0 = lua_tointeger(L,4);
	int x1 = lua_tointeger(L,5);
	int y1 = lua_tointeger(L,6);
	
	lua_newtable(L);
	find_path(L,pf,index,x0,y0,x1,y1);

	return 1;
}

int
_path_dump_map(lua_State *L) {
	struct pathfinder *pf = lua_touserdata(L,1);
	int index = lua_tointeger(L,2);
	struct map *mp = &pf->map_mgr[index];

	int i = 0;
	int j = 0;
	for(;i<mp->width;++i) {
		for(j =0;j<mp->heigh;++j) {		
			printf("%d",mp->data[i*mp->heigh+j]);
		}
		printf("\n");
	}
	return 0;
}
  
int
luaopen_pathfinder_core(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{"create", _path_create},
	  	{"release", _path_release},
	  	{"init", _path_init},
	  	{"find", _path_find},
	  	{"dump_map", _path_dump_map},
		{ NULL, NULL },
	};

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
