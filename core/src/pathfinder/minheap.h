struct element {
	int index;
};

struct minheap {
	int cap;
	int size;
	int (*less)(struct element *l,struct element *r);
	struct element **elts;
};

struct minheap *minheap_new(int,int (*less)(struct element *l,struct element *r));
void minheap_change(struct minheap *,struct element *);
void minheap_push(struct minheap *,struct element *);
struct element * minheap_pop(struct minheap *);
void minheap_clear(struct minheap *);