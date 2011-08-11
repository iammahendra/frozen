# This file was automatically generated by SWIG (http://www.swig.org).
# Version 2.0.4
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

package Frozen;
use base qw(Exporter);
use base qw(DynaLoader);
package Frozenc;
bootstrap Frozen;
package Frozen;
@EXPORT = qw();

# ---------- BASE METHODS -------------

package Frozen;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package Frozen;

*frozen_init = *Frozenc::frozen_init;
*frozen_destroy = *Frozenc::frozen_destroy;
*class_register = *Frozenc::class_register;
*class_unregister = *Frozenc::class_unregister;
*backend_new = *Frozenc::backend_new;
*backend_acquire = *Frozenc::backend_acquire;
*backend_find = *Frozenc::backend_find;
*backend_query = *Frozenc::backend_query;
*backend_destroy = *Frozenc::backend_destroy;
*backend_pass = *Frozenc::backend_pass;
*configs_string_parse = *Frozenc::configs_string_parse;
*configs_file_parse = *Frozenc::configs_file_parse;
*hash_new = *Frozenc::hash_new;
*hash_copy = *Frozenc::hash_copy;
*hash_free = *Frozenc::hash_free;
*hash_find = *Frozenc::hash_find;
*hash_chain = *Frozenc::hash_chain;
*hash_nelements = *Frozenc::hash_nelements;
*hash_dump = *Frozenc::hash_dump;
*hash_string_to_key = *Frozenc::hash_string_to_key;
*hash_key_to_string = *Frozenc::hash_key_to_string;
*hash_key_to_ctx_key = *Frozenc::hash_key_to_ctx_key;
*hash_item_key = *Frozenc::hash_item_key;
*hash_item_data = *Frozenc::hash_item_data;
*hash_item_next = *Frozenc::hash_item_next;
*hash_data_find = *Frozenc::hash_data_find;
*data_type_from_string = *Frozenc::data_type_from_string;
*data_string_from_type = *Frozenc::data_string_from_type;
*data_free = *Frozenc::data_free;
*hash_get = *Frozenc::hash_get;
*hash_set = *Frozenc::hash_set;
*data_from_string = *Frozenc::data_from_string;
*describe_error = *Frozenc::describe_error;

############# Class : Frozen::backend_t ##############

package Frozen::backend_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Frozen );
%OWNER = ();
%ITERATORS = ();
*swig_name_get = *Frozenc::backend_t_name_get;
*swig_name_set = *Frozenc::backend_t_name_set;
*swig_class_get = *Frozenc::backend_t_class_get;
*swig_class_set = *Frozenc::backend_t_class_set;
*swig_supported_api_get = *Frozenc::backend_t_supported_api_get;
*swig_supported_api_set = *Frozenc::backend_t_supported_api_set;
*swig_func_init_get = *Frozenc::backend_t_func_init_get;
*swig_func_init_set = *Frozenc::backend_t_func_init_set;
*swig_func_configure_get = *Frozenc::backend_t_func_configure_get;
*swig_func_configure_set = *Frozenc::backend_t_func_configure_set;
*swig_func_fork_get = *Frozenc::backend_t_func_fork_get;
*swig_func_fork_set = *Frozenc::backend_t_func_fork_set;
*swig_func_destroy_get = *Frozenc::backend_t_func_destroy_get;
*swig_func_destroy_set = *Frozenc::backend_t_func_destroy_set;
*swig_backend_type_crwd_get = *Frozenc::backend_t_backend_type_crwd_get;
*swig_backend_type_crwd_set = *Frozenc::backend_t_backend_type_crwd_set;
sub new {
    my $pkg = shift;
    my $self = Frozenc::new_backend_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Frozenc::delete_backend_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : Frozen::backend_t_backend_type_crwd ##############

package Frozen::backend_t_backend_type_crwd;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( Frozen );
%OWNER = ();
%ITERATORS = ();
*swig_func_create_get = *Frozenc::backend_t_backend_type_crwd_func_create_get;
*swig_func_create_set = *Frozenc::backend_t_backend_type_crwd_func_create_set;
*swig_func_set_get = *Frozenc::backend_t_backend_type_crwd_func_set_get;
*swig_func_set_set = *Frozenc::backend_t_backend_type_crwd_func_set_set;
*swig_func_get_get = *Frozenc::backend_t_backend_type_crwd_func_get_get;
*swig_func_get_set = *Frozenc::backend_t_backend_type_crwd_func_get_set;
*swig_func_delete_get = *Frozenc::backend_t_backend_type_crwd_func_delete_get;
*swig_func_delete_set = *Frozenc::backend_t_backend_type_crwd_func_delete_set;
*swig_func_move_get = *Frozenc::backend_t_backend_type_crwd_func_move_get;
*swig_func_move_set = *Frozenc::backend_t_backend_type_crwd_func_move_set;
*swig_func_count_get = *Frozenc::backend_t_backend_type_crwd_func_count_get;
*swig_func_count_set = *Frozenc::backend_t_backend_type_crwd_func_count_set;
*swig_func_custom_get = *Frozenc::backend_t_backend_type_crwd_func_custom_get;
*swig_func_custom_set = *Frozenc::backend_t_backend_type_crwd_func_custom_set;
sub new {
    my $pkg = shift;
    my $self = Frozenc::new_backend_t_backend_type_crwd(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        Frozenc::delete_backend_t_backend_type_crwd($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package Frozen;

*HK_action = *Frozenc::HK_action;
*HK_action_global = *Frozenc::HK_action_global;
*HK_action_one = *Frozenc::HK_action_one;
*HK_action_perfork = *Frozenc::HK_action_perfork;
*HK_action_request = *Frozenc::HK_action_request;
*HK_action_request_global = *Frozenc::HK_action_request_global;
*HK_action_request_one = *Frozenc::HK_action_request_one;
*HK_action_request_perfork = *Frozenc::HK_action_request_perfork;
*HK_after = *Frozenc::HK_after;
*HK_async = *Frozenc::HK_async;
*HK_backend = *Frozenc::HK_backend;
*HK_backend_e = *Frozenc::HK_backend_e;
*HK_backend_g = *Frozenc::HK_backend_g;
*HK_backend_v = *Frozenc::HK_backend_v;
*HK_backends = *Frozenc::HK_backends;
*HK_before = *Frozenc::HK_before;
*HK_block_off = *Frozenc::HK_block_off;
*HK_block_size = *Frozenc::HK_block_size;
*HK_block_vid = *Frozenc::HK_block_vid;
*HK_blocks = *Frozenc::HK_blocks;
*HK_buffer = *Frozenc::HK_buffer;
*HK_buffer_size = *Frozenc::HK_buffer_size;
*HK_class = *Frozenc::HK_class;
*HK_clone = *Frozenc::HK_clone;
*HK_config = *Frozenc::HK_config;
*HK_copy = *Frozenc::HK_copy;
*HK_count = *Frozenc::HK_count;
*HK_create = *Frozenc::HK_create;
*HK_dns_domain = *Frozenc::HK_dns_domain;
*HK_dns_ip = *Frozenc::HK_dns_ip;
*HK_dns_tstamp = *Frozenc::HK_dns_tstamp;
*HK_engine = *Frozenc::HK_engine;
*HK_exclusive = *Frozenc::HK_exclusive;
*HK_field = *Frozenc::HK_field;
*HK_filename = *Frozenc::HK_filename;
*HK_force_async = *Frozenc::HK_force_async;
*HK_force_sync = *Frozenc::HK_force_sync;
*HK_forced = *Frozenc::HK_forced;
*HK_fork = *Frozenc::HK_fork;
*HK_fork_only = *Frozenc::HK_fork_only;
*HK_fork_request = *Frozenc::HK_fork_request;
*HK_function = *Frozenc::HK_function;
*HK_handle = *Frozenc::HK_handle;
*HK_hash = *Frozenc::HK_hash;
*HK_homedir = *Frozenc::HK_homedir;
*HK_insert = *Frozenc::HK_insert;
*HK_item_size = *Frozenc::HK_item_size;
*HK_key = *Frozenc::HK_key;
*HK_key1 = *Frozenc::HK_key1;
*HK_key2 = *Frozenc::HK_key2;
*HK_key3 = *Frozenc::HK_key3;
*HK_key4 = *Frozenc::HK_key4;
*HK_key_from = *Frozenc::HK_key_from;
*HK_key_out = *Frozenc::HK_key_out;
*HK_key_to = *Frozenc::HK_key_to;
*HK_keyid = *Frozenc::HK_keyid;
*HK_linear_len = *Frozenc::HK_linear_len;
*HK_max_global = *Frozenc::HK_max_global;
*HK_max_perfork = *Frozenc::HK_max_perfork;
*HK_max_perinstance = *Frozenc::HK_max_perinstance;
*HK_max_rebuilds = *Frozenc::HK_max_rebuilds;
*HK_mode = *Frozenc::HK_mode;
*HK_mode_global = *Frozenc::HK_mode_global;
*HK_mode_perfork = *Frozenc::HK_mode_perfork;
*HK_multiply = *Frozenc::HK_multiply;
*HK_n_initial = *Frozenc::HK_n_initial;
*HK_name = *Frozenc::HK_name;
*HK_nelements = *Frozenc::HK_nelements;
*HK_nelements_min = *Frozenc::HK_nelements_min;
*HK_nelements_mul = *Frozenc::HK_nelements_mul;
*HK_nelements_step = *Frozenc::HK_nelements_step;
*HK_offset = *Frozenc::HK_offset;
*HK_offset_from = *Frozenc::HK_offset_from;
*HK_offset_out = *Frozenc::HK_offset_out;
*HK_offset_to = *Frozenc::HK_offset_to;
*HK_on_destroy = *Frozenc::HK_on_destroy;
*HK_on_end = *Frozenc::HK_on_end;
*HK_on_postreq = *Frozenc::HK_on_postreq;
*HK_on_prereq = *Frozenc::HK_on_prereq;
*HK_on_start = *Frozenc::HK_on_start;
*HK_param = *Frozenc::HK_param;
*HK_parameter = *Frozenc::HK_parameter;
*HK_parameter_request = *Frozenc::HK_parameter_request;
*HK_path = *Frozenc::HK_path;
*HK_perlevel = *Frozenc::HK_perlevel;
*HK_pool = *Frozenc::HK_pool;
*HK_pool_interval = *Frozenc::HK_pool_interval;
*HK_pool_size = *Frozenc::HK_pool_size;
*HK_post_request = *Frozenc::HK_post_request;
*HK_pre_request = *Frozenc::HK_pre_request;
*HK_random = *Frozenc::HK_random;
*HK_read_size = *Frozenc::HK_read_size;
*HK_readonly = *Frozenc::HK_readonly;
*HK_real_offset = *Frozenc::HK_real_offset;
*HK_request = *Frozenc::HK_request;
*HK_ret = *Frozenc::HK_ret;
*HK_retry = *Frozenc::HK_retry;
*HK_role = *Frozenc::HK_role;
*HK_script = *Frozenc::HK_script;
*HK_size = *Frozenc::HK_size;
*HK_size_old = *Frozenc::HK_size_old;
*HK_string = *Frozenc::HK_string;
*HK_structure = *Frozenc::HK_structure;
*HK_tick_interval = *Frozenc::HK_tick_interval;
*HK_type = *Frozenc::HK_type;
*HK_value = *Frozenc::HK_value;
*HK_value_bits = *Frozenc::HK_value_bits;
*HK_values = *Frozenc::HK_values;
*HK_verbose = *Frozenc::HK_verbose;
*TYPE_INVALID = *Frozenc::TYPE_INVALID;
*TYPE_BACKENDT = *Frozenc::TYPE_BACKENDT;
*TYPE_BINARYT = *Frozenc::TYPE_BINARYT;
*TYPE_BUFFERT = *Frozenc::TYPE_BUFFERT;
*TYPE_HASHT = *Frozenc::TYPE_HASHT;
*TYPE_IOT = *Frozenc::TYPE_IOT;
*TYPE_MEMORYT = *Frozenc::TYPE_MEMORYT;
*TYPE_RAWT = *Frozenc::TYPE_RAWT;
*TYPE_STRINGT = *Frozenc::TYPE_STRINGT;
*TYPE_STRUCTT = *Frozenc::TYPE_STRUCTT;
*TYPE_OFFT = *Frozenc::TYPE_OFFT;
*TYPE_SIZET = *Frozenc::TYPE_SIZET;
*TYPE_UINTT = *Frozenc::TYPE_UINTT;
*TYPE_INTT = *Frozenc::TYPE_INTT;
*TYPE_INT8T = *Frozenc::TYPE_INT8T;
*TYPE_INT16T = *Frozenc::TYPE_INT16T;
*TYPE_INT32T = *Frozenc::TYPE_INT32T;
*TYPE_INT64T = *Frozenc::TYPE_INT64T;
*TYPE_UINT8T = *Frozenc::TYPE_UINT8T;
*TYPE_UINT16T = *Frozenc::TYPE_UINT16T;
*TYPE_UINT32T = *Frozenc::TYPE_UINT32T;
*TYPE_UINT64T = *Frozenc::TYPE_UINT64T;
*TYPE_VOIDT = *Frozenc::TYPE_VOIDT;
*ACTION_CRWD_CREATE = *Frozenc::ACTION_CRWD_CREATE;
*ACTION_CRWD_READ = *Frozenc::ACTION_CRWD_READ;
*ACTION_CRWD_WRITE = *Frozenc::ACTION_CRWD_WRITE;
*ACTION_CRWD_DELETE = *Frozenc::ACTION_CRWD_DELETE;
*ACTION_CRWD_MOVE = *Frozenc::ACTION_CRWD_MOVE;
*ACTION_CRWD_COUNT = *Frozenc::ACTION_CRWD_COUNT;
*ACTION_CRWD_CUSTOM = *Frozenc::ACTION_CRWD_CUSTOM;
*REQUEST_INVALID = *Frozenc::REQUEST_INVALID;

INIT    { frozen_init();  }
DESTROY { frozen_destroy(); }

sub query {
	my $backend = shift;
	my $request = shift;
	my $code    = (shift or undef);
	
	my ($arrsz, $h_request, $ret);
	
	$arrsz = (scalar keys %$request) + 1;
	
	$h_request = hash_new($arrsz);
	while(my ($k,$va) = each(%$request)){
		my ($t,$v) = %$va;
		
		hash_set($h_request, $k, $t, "$v");
	}
	
	$ret = backend_query($backend, $h_request);
	
	if(defined $code){
		&$code($h_request);
	}
	
	hash_free($h_request);
	
	return $ret;
}
1;
