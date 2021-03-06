#include <libfrozen.h>
#include <dataproto.h>
#include <string_t.h>
#include <format/format_t.h>

static ssize_t data_string_t_physlen(data_t *data, fastcall_physicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr) + 1;
	return 0;
} // }}}
static ssize_t data_string_t_loglen(data_t *data, fastcall_logicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr);
	return 0;
} // }}}
static ssize_t data_string_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                  *buffer            = NULL;
	uintmax_t              buffer_size       = 0;
	uintmax_t              malloc_size;
	uintmax_t              format            = FORMAT(clean);
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(fargs->header.nargs > 3)
		format = fargs->format;
	
	// fast convert from string (string can only contain FORMAT(clean))
	if(fargs->src->type == TYPE_STRINGT){
		dst->ptr = strdup((char *)fargs->src->ptr);
		return 0;
	}
	
	// get our buffer buffer
	buffer = dst->ptr;
	if(buffer){
		buffer_size = strlen(buffer); // existing buffer - hell know where is it, so, no reallocs
		goto clean_read;
	}
	
	// get external buffer size
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(fargs->src, &r_len) != 0)
		goto unknown_size;
	
	// alloc new buffer
	switch(format){
		case FORMAT(clean):         buffer_size = r_len.length;     malloc_size = r_len.length + 1; break;
		case FORMAT(human): buffer_size = r_len.length;     malloc_size = r_len.length + 1; break;
		case FORMAT(binary):        if(r_len.length == 0)
						return -EINVAL;   
					   buffer_size = r_len.length - 1; malloc_size = r_len.length;     break;
		default:
			return -ENOSYS;
	};
	
	if( (buffer = malloc(malloc_size)) == NULL)
		return -ENOMEM;
	
	buffer[buffer_size] = '\0';
	
	dst->ptr = buffer;
	goto clean_read;

unknown_size:
	
	/*
	buffer_size = DEF_BUFFER_SIZE;
	
	while(1){
		buffer = realloc(buffer, buffer_size);
		
		fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, buffer_size };
		if( (ret = data_query(fargs->src, &r_read)) < 0)
			return ret;
		

		buffer_size += DEF_BUFFER_SIZE;
	}
	*/
	return -ENOSYS;

clean_read:;
	fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, buffer_size };
	if( (ret = data_query(fargs->src, &r_read)) < -1)
		return ret;
		
	return 0;
} // }}}
static ssize_t data_string_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered;
	uintmax_t              buffer_size;
	
	if(fargs->dest == NULL || !src->ptr)
		return -EINVAL;
	
	buffer_size = strlen(src->ptr);
	
	switch(fargs->format){
		case FORMAT(binary):;
			fastcall_write r_write1 = { { 5, ACTION_WRITE }, 0, src->ptr, buffer_size + 1 };
			ret        = data_query(fargs->dest, &r_write1);
			transfered = r_write1.buffer_size;
			break;
		
		case FORMAT(clean):;
		case FORMAT(human):;
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, 0, src->ptr, buffer_size };
			ret        = data_query(fargs->dest, &r_write2);
			transfered = r_write2.buffer_size;
			break;
		
		default:
			return -ENOSYS;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}

data_proto_t string_t_proto = {
	.type          = TYPE_STRINGT,
	.type_str      = "string_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_string_t_physlen,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_string_t_loglen,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_string_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_string_t_convert_from,
	}
};
