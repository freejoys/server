
struct list_node {
	struct list_node * pre;
    struct list_node * next;
};

struct list {
	struct list_node head;
	struct list_node tail;
};

static inline void
list_init(struct list * dl) {
	dl->head.pre = dl->tail.next = NULL;
	dl->head.next = &dl->tail;
	dl->tail.pre = &dl->head;
}

static inline int
list_empty(struct list * dl) {
	return dl->head.next == &dl->tail ? 1:0;
}

static inline struct list_node*
list_head(struct list * dl) {
	return dl->head.next;
}

static inline struct list_node*
list_tail(struct list * dl) {
	return &dl->tail;
}

static inline int
list_remove(struct list_node * node) {
	if (!node->pre || !node->next)
		return 0;
	node->pre->next = node->next;
	node->next->pre = node->pre;
	node->pre = node->next = NULL;
	return 0;
}

static inline struct list_node *
list_pop(struct list * dl) {
	struct list_node * node = NULL;
	if (!list_empty(dl)) {
		node = dl->head.next;
		list_remove(node);
	}
	return node;
}

static inline int
list_push(struct list * dl,struct list_node * node) {
	if (node->pre != NULL || node->next != NULL) {
		return -1;
	}
	dl->tail.pre->next = node;
	node->pre = dl->tail.pre;
	dl->tail.pre = node;
	node->next = &dl->tail;
	return 0;
}

static inline int
list_push_head(struct list * dl,struct list_node * node) {
	if (node->pre != NULL || node->next != NULL) {
		return -1;
	}
	struct list_node * next = dl->head.next;
	dl->head.next = node;
	node->pre = &dl->head;
	node->next = next;
	next->pre = node;
	return 0;
}