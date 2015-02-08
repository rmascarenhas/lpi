#include "tsbintree.h"

#define PthreadCheck(status) do { \
	if (status != 0) { \
		errno = status; \
		return -1; \
	} \
} while (0);

#define UnlockNode(bt, ivar) do { \
	ivar = pthread_mutex_unlock(&bt->lock); \
	PthreadCheck(ivar); \
} while (0);

#define Free(node) do { \
	node->key = node->value = NULL; \
} while (0);


int
tsbintree_init(tsbintree *bt) {
	int s = pthread_mutex_init(&bt->lock, NULL);
	PthreadCheck(s);

	s = pthread_mutex_lock(&bt->lock);
	PthreadCheck(s);

	bt->key = NULL;
	bt->value = NULL;
	bt->left = bt->right = NULL;

	UnlockNode(bt, s);
	return s;
}

int
tsbintree_add(tsbintree *bt, char *key, void *value) {
	if (key == NULL) {
		errno = EINVAL;
		return -1;
	}

	int r = -1;
	int s = pthread_mutex_lock(&bt->lock);
	PthreadCheck(s);

	/* base case for the recursion */
	if (bt->key == NULL) {
		bt->key = key;
		bt->value = value;

		UnlockNode(bt, s);
		return s;
	}

	/* if key is smaller than the current key, insert it at the left side;
	 * otherwise insert it at the rigth */
	int cmp = strncmp(bt->key, key, TSBT_MAX_KEY_SIZE);
	if (cmp > 0) {
		if (bt->left == NULL) {
			bt->left = malloc(sizeof(tsbintree));
			tsbintree_init(bt->left);
		}

		UnlockNode(bt, s);
		r = tsbintree_add(bt->left, key, value);
	} else if (cmp < 0) {
		if (bt->right == NULL) {
			bt->right = malloc(sizeof(tsbintree));
			tsbintree_init(bt->right);
		}

		UnlockNode(bt, s);
		r =  tsbintree_add(bt->right, key, value);
	} else {
		UnlockNode(bt, s);
		errno = EINVAL;
	}

	return r;
}

int
tsbintree_lookup(tsbintree *bt, char *key, void **value) {
	if (strlen(key) > TSBT_MAX_KEY_SIZE) {
		errno = EINVAL;
		return -1;
	}


	int r, s, cmp;
	s = pthread_mutex_lock(&bt->lock);
	PthreadCheck(s);

	/* recursion base case */
	if (bt == NULL || bt->key == NULL) {
		UnlockNode(bt, s);
		return 0;
	}

	cmp = strncmp(bt->key, key, TSBT_MAX_KEY_SIZE);
	UnlockNode(bt, s);

	if (cmp > 0) {
		r = tsbintree_lookup(bt->left, key, value);
	} else if (cmp > 0) {
		r = tsbintree_lookup(bt->right, key, value);
	} else {
		*value = bt->value;
		r = 0;
	}

	return r;
}

static int
find_successor(tsbintree *root, tsbintree **successor, tsbintree **successor_parent) {
	tsbintree *p, *parent;
	int s;

	p = root;
	parent = NULL;

	s = pthread_mutex_lock(&p->lock);
	PthreadCheck(s);

	while (p->right) {
		parent = p;
		p = p->right;

		UnlockNode(parent, s);
		s = pthread_mutex_lock(&p->lock);
		PthreadCheck(s);
	}
	UnlockNode(p, s);

	*successor = p;
	*successor_parent = parent;

	return 0;
}

/* based on http://webdocs.cs.ualberta.ca/~holte/T26/del-from-bst.html */
int
tsbintree_delete(tsbintree *bt, char *key) {
	if (key == NULL) {
		errno = EINVAL;
		return -1;
	}

	int s;
	s = pthread_mutex_lock(&bt->lock);
	PthreadCheck(s);

	if (bt == NULL || key == NULL) {
		errno = EINVAL;
		UnlockNode(bt, s);
		return -1;
	}

	/* deleting from a recently initialized tree */
	if (bt->key == NULL) {
		UnlockNode(bt, s);
		errno = ENOKEY;
		return -1;
	}

	tsbintree *p, *parent, *successor, *successor_parent, *tmp;
	int cmp;
	UnlockNode(bt, s);

	p = bt;
	parent = NULL;
	while (p) {
		s = pthread_mutex_lock(&p->lock);
		PthreadCheck(s);

		cmp = strncmp(p->key, key, TSBT_MAX_KEY_SIZE);

		if (cmp > 0) {
			UnlockNode(p, s);
			parent = p;
			p = p->left;
		} else if (cmp < 0) {
			UnlockNode(p, s);
			parent = p;
			p = p->right;
		} else {
			/* key found: delete it */
			if (parent) {
				assert(parent->left == p || parent->right == p);

				s = pthread_mutex_lock(&parent->lock);
				PthreadCheck(s);
			}

			/* deleted node is a leaf */
			if (!p->left && !p->right) {
				if (parent && parent->left == p)
					parent->left = NULL;
				else if (parent && parent->right == p)
					parent->right = NULL;

				Free(p);
			} else if (p->left && !p->right) {
				if (parent) {
					if (parent->left == p)
						parent->left = p->left;
					else if (parent->right == p)
						parent->right = p->left;

					Free(p);
				} else {
					tmp = p->left;

					/* copy over new root */
					p->key = p->left->key;
					p->value = p->left->value;
					p->left = p->left->left;
					p->right = p->left->right;

					Free(tmp);
				}
			} else if (!p->left && p->right) {
				if (parent) {
					if (parent->left == p)
						parent->left = p->right;
					else if (parent->right == p)
						parent->right = p->right;

					Free(p);
				} else {
					tmp = p->right;

					/* copy over new root */
					p->key = p->right->key;
					p->value = p->right->value;
					p->left = p->right->left;
					p->right = p->right->right;

					Free(tmp);
				}

			} else {
				/* deleted node has both subtrees */
				assert(find_successor(p->left, &successor, &successor_parent) != -1);
				p->key = successor->key;
				p->value = successor->value;

				if (successor_parent)
					successor_parent->right = successor->left;
				else
					p->left = NULL;

				Free(successor);
			}

			UnlockNode(p, s);
			if (parent) UnlockNode(parent, s);
			return 0;
		}
	}

	errno = ENOKEY;
	return -1;
}

int
tsbintree_destroy(tsbintree *root) {
	/* recursion base case */
	if (root == NULL || root->key == NULL)
		return 0;

	tsbintree_destroy(root->left);
	tsbintree_destroy(root->right);

	int s = pthread_mutex_destroy(&root->lock);
	PthreadCheck(s);

	return 0;
}

#ifdef TSBT_DEBUG
static int
nextid(char *id) {
	int len, i, inc, start = 0;

	inc = 0;
	len = strlen(id);

	if (id[len - 1] == 'Z') {
		for (i = len - 2; i >= 0 && !inc; --i) {
			if (id[i] != 'Z') {
				id[i]++;
				start = i + 1;
				inc = 1;
			}
		}
	} else {
		id[len - 1]++;
		return 0;
	}

	for (i = start; i < len; ++i)
		id[i] = 'A';

	if (!inc) {
		len++;
		if (len >= TSBT_MAX_DOT_LABEL_SIZE)
			return -1;


		id[len - 1] = 'A';
		id[len] = '\0';
	}

	return 0;
}

static int
tsbintree_to_dot_rec(tsbintree *root, char *buffer, int size, int script_size, char *id) {
#define AppendToBuffer(...) do { \
	tmp = snprintf(&buffer[pos], size - pos, __VA_ARGS__); \
	pos += tmp; \
	len += tmp; \
	if (pos >= size) { \
		errno = ENOMEM; \
		return -1; \
	} \
} while (0);

#define NextId(id) do { \
	if (nextid(id) == -1) \
		return -1; \
} while (0);

	int pos = script_size,
	    len = 0,
	    left, right, tmp;

	char rootid[TSBT_MAX_DOT_LABEL_SIZE];
	strncpy(rootid, id, TSBT_MAX_DOT_LABEL_SIZE);

	/* print its own label */
	if (root == NULL || root->key == NULL) {
		/* print a single point to indicate a NULL child */
		AppendToBuffer("%s[shape=point];", id);
		NextId(id);
		return len;
	}

	AppendToBuffer("%s[label=%s];", id, root->key);
	NextId(id);

	/* print first left subtree then right subtree */
	AppendToBuffer("%s--%s;", rootid, id);
	left = tsbintree_to_dot_rec(root->left, buffer, size, pos, id);
	if (left == -1)
		return -1;

	pos += left;
	AppendToBuffer("%s--%s;", rootid, id);
	right = tsbintree_to_dot_rec(root->right, buffer, size, pos, id);
	if (right == -1)
		return -1;

	return (len + left + right);
}

int
tsbintree_to_dot(tsbintree *bt, char *buffer, int size) {
	int written = 0;
	char label[TSBT_MAX_DOT_LABEL_SIZE] = "A";

	if (bt == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (size < TSBT_DOT_HEADER_LEN) {
		errno = ENOMEM;
		return -1;
	}

	snprintf(buffer, size, TSBT_DOT_HEADER);
	if ((written = tsbintree_to_dot_rec(bt, buffer, size, TSBT_DOT_HEADER_LEN, label)) == -1) {
		errno = ENOMEM;
		return -1;
	}

	if (size < written + TSBT_DOT_HEADER_LEN + 1) {
		errno = ENOMEM;
		return -1;
	}

	snprintf(&buffer[TSBT_DOT_HEADER_LEN + written], size, "}");
	return size;
}

int
tsbintree_print(tsbintree *bt) {
	int nodes = 0;

	if (bt != NULL && bt->key != NULL) {
		++nodes;

		nodes += tsbintree_print(bt->left);
		printf("\t* %s=%s\n", bt->key, (char *) bt->value);
		nodes += tsbintree_print(bt->right);
	}

	return nodes;
}
#endif
