/*
	Copyright (C) 2015 Smx
*/
#include <grub/file.h>
#include <stdarg.h>
#include <grub/misc.h>
#include <grub/memory.h>
#include <grub/lib/plist.h>
#include <grub/types.h>

#ifndef SEEK_SET
# define SEEK_SET   0   /* Seek from beginning of file.  */
#endif
#ifndef SEEK_CUR
# define SEEK_CUR   1   /* Seek from current position.  */
#endif
#ifndef SEEK_END
# define SEEK_END   2   /* Seek from end of file.  */
#endif


#define PLIST_READTO(plist, end) plist_readTo(plist, (char *)end, grub_strlen(end))
#define strlencmp(x, y) grub_strncmp(x, y, grub_strlen(y))

const int plist_debug = true;

void debug(const char *format, ...);
void plist_node_free(PLIST_NODE *node);


int plist_readTo(PLIST *plist, char *to, int size){
	int count;
	for(count=0; ; plist->offset++, plist->data++, count++){
		if(!grub_strncmp(plist->data, to, size)){
			return count-1;
		}
		if(plist_feof(plist)){
			return -1;
		}
	}
}

int plist_nextLine(PLIST *plist){
	int ret = PLIST_READTO(plist, "\n");
	plist->data++;
	if(++plist->offset > plist->size || !ret){
		return ret;
	}
	return true;
}

void plist_rewind(PLIST *plist){
	plist->offset = 0;
	plist->data = plist->dataStart;
}

int plist_fgetc(PLIST *plist){
	if(plist_feof(plist)){
		return -1;
	}
	plist->data++; plist->offset++;
	return (char)*(plist->data);
}

int plist_feof(PLIST *plist){
	if(plist->offset >= plist->size){
		return true;
	}
	return false;
}

void debug(const char *format, ...){
	if(plist_debug){
		va_list args;
		va_start(args, format);
		grub_vprintf(format, args);
		va_end(args);
	}
}

int plist_fseek(PLIST *plist, int offset, int origin){
	switch(origin){
		case SEEK_SET:
			plist->offset = offset;
			plist->data = plist->dataStart + offset;
			break;
		case SEEK_CUR:
			plist->offset += offset;
			plist->data += offset;
			break;
		case SEEK_END:
			plist->offset = plist->size + offset;
			plist->data = plist->dataStart + plist->size + offset;
			break;
		default:
			return false;
			break;
	}
	return true;
}

int plist_isValid(PLIST *plist){
	int plistCheckLevel = 0;
	PLIST_READTO(plist, "<");
	if(!strlencmp(plist->data, "<?xml")){
		debug("<?xml matches\n");
		plistCheckLevel++;
		if(!plist_nextLine(plist)) return false;
	}
	if(!strlencmp(plist->data, "<!DOCTYPE plist PUBLIC")){
		debug("<!DOCTYPE plist PUBLIC matches\n");
		plistCheckLevel++;
		if(!plist_nextLine(plist)) return false;
	}
	int readCount = PLIST_READTO(plist, "<plist");
	if(!strlencmp(plist->data, "<plist") || readCount <= 0){
		debug("<plist matches\n");
		plistCheckLevel++;
	} else {
		debug("Fatal!, cannot find the root node!\n");
		return false;
	}
	if(plistCheckLevel < 1) return false;
	debug("Alright then, it's a valid plist :)\n");
	return true;
}

int plist_text2node(PLIST *plist, PLIST_NODE *prev, NODE_TYPE type){
	debug("\n");
	if(*(plist->data) != '<'){
		return false;
	}
	PLIST_NODE *node = grub_zalloc(sizeof(PLIST_NODE));
	
	if(!plist->root || !prev){
		debug("setting root node\n");
		plist->root = node;
	} else {
		switch(type){
			case NODE_RIGHT:
				debug("setting next node\n");
				prev->next = node;
				debug("setting prev node\n");
				node->prev = prev;
				debug("copying parent node\n");
				node->parent = prev->parent;
				break;
			case NODE_CHILD:
				debug("setting child node\n");
				prev->children = node;
				debug("setting parent node\n");
				node->parent = prev;
				break;
			case NODE_LEFT:
			case NODE_PARENT:
			default:
				break;
		}
	}
	
	
	int tagNameSize = PLIST_READTO(plist, ">");
	debug("Tagsize: %d\n",tagNameSize); 
	if(tagNameSize <= 0){
		return false;
	}
	
	plist_fseek(plist, -tagNameSize, SEEK_CUR);
	char *tagName = grub_zalloc(tagNameSize);
	grub_strncpy(tagName, plist->data, tagNameSize);
	debug("Tag: %s\n", tagName);
	node->tagName = tagName;
	plist_fseek(plist, tagNameSize, SEEK_CUR);
	int countentSize = PLIST_READTO(plist, "<");
	if(plist_fgetc(plist) != '/'){ //it's not tag end. a children starts
		debug("child node detected\n");
		plist_fseek(plist, -1, SEEK_CUR);
		plist_text2node(plist, node, NODE_CHILD);
	} else {
		plist_fseek(plist, -(1+countentSize), SEEK_CUR);
		char *textContent = grub_zalloc(countentSize);
		grub_strncpy(textContent, plist->data, countentSize);
		debug("Content: %s\n", textContent);
		node->textContent = textContent;
	}
	while(!plist_feof(plist)){
		int c = PLIST_READTO(plist, "<");
		if(c < 0) break;
		if(plist_fgetc(plist) == '/'){
			if(!strlencmp(plist->data, tagName)){
				break; //we reached the end of the level (right/left nodes)
			} else {
				continue; //it's the end of a tag in the same level
			}
		}
		debug("right node detected\n");
		plist_fseek(plist, -1, SEEK_CUR);
		plist_text2node(plist, node, NODE_RIGHT);
	}
	return true;
}

PLIST_NODE *plist_getNodebyField(PLIST_NODE *node, NODE_FIELD field, const char *fieldValue){
	debug("%s: ", node->tagName);
	if(node->textContent){
		debug("%s", node->textContent);
	}
	debug("\n");
	switch(field){
		case NODE_TAGNAME:
			if(node->tagName && !strlencmp(node->tagName, fieldValue)){
				return node;
			}
			break;
		case NODE_TEXTCONTENT:
			if(node->textContent && !strlencmp(node->textContent, fieldValue)){
				return node;
			}
			break;
	}
	PLIST_NODE *result;
	if(node->next){
		debug("going next\n");
		result = plist_getNodebyField(node->next, field, fieldValue);
		if(result){
			return result;
		}
	}
	if(node->children){
		debug("going child %s\n");
		result = plist_getNodebyField(node->children, field, fieldValue);
		if(result){
			return result;
		}
	}
	return false;
}

PLIST_NODE *plist_getNodeByTagName(PLIST *plist, const char *tagName, PLIST_NODE *prev){
	if(prev){
		return plist_getNodebyField(prev, NODE_TAGNAME, tagName);
	} else {
		return plist_getNodebyField(plist->root, NODE_TAGNAME, tagName);
	}
}

PLIST_NODE *plist_getNodeByTextContent(PLIST *plist, const char *textContent, PLIST_NODE *prev){
	if(prev){
		return plist_getNodebyField(prev, NODE_TEXTCONTENT, textContent);
	} else {
		return plist_getNodebyField(plist->root, NODE_TEXTCONTENT, textContent);
	}
}

PLIST_NODE *plist_getNodeByKey(PLIST *plist, const char *keyName, PLIST_NODE *prev){
	PLIST_NODE *result;
	if(prev){
		result = plist_getNodeByTextContent(plist, keyName, prev);
	} else {
		result = plist_getNodeByTextContent(plist, keyName, plist->root);
	}
	if(result && result->next){
		return result->next;
	}
	return (PLIST_NODE *)false;
}

void plist_node_free(PLIST_NODE *node){
	if(node && node->next){
		debug("going next\n");
		plist_node_free(node->next);
	}
	if(node && node->children){
		debug("going child %s\n");
		plist_node_free(node->children);
	} else {
		if(node){
			debug("free call\n");
			grub_free(node);
		}
	}
}

int plist_close(PLIST *plist){
	if(plist){
		if(plist->data){
			grub_free(plist->data);
		}
		if(plist->root){
			plist_node_free(plist->root);
		}
		grub_free(plist);
	} else {
		return false;
	}
	return true;
}

int plist_open(const char *filename, PLIST *plist){
	grub_file_t file;
	
	file = grub_file_open(filename);
	if(!file){
		debug("open fail\n");
		goto clean_return;
	}
	plist->size = grub_file_size(file);
	if(plist->size <= 0){
		debug("size inval\n");
		goto clean_return;
	}
	plist->data = grub_malloc(plist->size);
	if(!plist->data){
		debug("malloc fail\n");
		goto clean_return;
	}
	plist->dataStart = plist->data;
	if(grub_file_read (file, plist->data, plist->size) != (grub_ssize_t) (plist->size)){
		grub_error(GRUB_ERR_BAD_OS, N_("premature end of file %s"), filename);
		goto clean_return;
	}
	if(!(plist->data)){
		debug("data fail\n");
		goto clean_return;
	}
	if(plist_isValid(plist)){
		return plist_text2node(plist, 0, NODE_PARENT);
	} else {
		debug("verif fail\n");
		goto clean_return;
	}
	return 1;
		
	clean_return:
		if(file){
			grub_file_close(file);
		}
		if(plist){
			plist_close(plist);
		}
		return 0;
}

