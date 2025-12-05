#include "identifier.c"

typedef struct {
	TypeStateActions actions;
	Type* type;
} OpenedType;

extern int in_compiler_step;
extern Compiler* generics_compiler_context;
TypeStateActions global_state_actions = { 0 };

enum {
	ActionKeepGlobalState = 1 << 2,
	ActionNoChildCompilation = 1 << 3,
};

OpenedType open_type_wa(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
		void* accumulator, unsigned flags);

#define open_type(type, flags) open_type_wa(type, NULL, NULL, NULL, flags);

int type_state_action(TypeStateAction action, unsigned flags) {
	switch(action.type) {
		case StateActionGenerics:
			for(size_t i = 0; i < action.TypeList.size; i++) {
				if(action.TypeList.data[i]->compiler == (void*) &comp_Wrapper
						&& action.TypeList.data[i]->Wrapper.anchor
						&& action.TypeList.data[i]->Wrapper.anchor->declaration
						== (void*) action.target) {
					return 0;
				}
			}

			push(&action.target->Declaration.generics.stack, action.TypeList);

			static int noloop = 1;

			if(in_compiler_step && noloop && !(flags & ActionNoChildCompilation)) {
				noloop = 0;
				str unique_key = { 0 };
				append_generics_identifier(&unique_key, action.TypeList);
				noloop = 1;

				if(!get(action.target->Declaration.generics.unique_map,
							(Trace) { .slice = unique_key })) {
					put(&action.target->Declaration.generics.unique_map, unique_key, NULL);
					action.target->compiler(action.target, NULL, generics_compiler_context);
				}
			}

			break;

		case StateActionCollection:
			for(size_t i = 0; i < action.TypeStateActions.size; i++) {
				type_state_action(action.TypeStateActions.data[i],
						flags | ActionKeepGlobalState);
			}
			break;
	}

	if(!(flags & ActionKeepGlobalState)) {
		push(&global_state_actions, action);
	}

	return 1;
}

void undo_type_state_action(TypeStateAction action, unsigned flags) {
	if(!(flags & ActionKeepGlobalState)) {
		global_state_actions.size--;
	}

	switch(action.type) {
		case StateActionGenerics:
			action.target->Declaration.generics.stack.size--;
			break;

		case StateActionCollection:
			for(size_t i = action.TypeStateActions.size; i > 0; i--) {
				undo_type_state_action(action.TypeStateActions.data[i - 1],
						flags | ActionKeepGlobalState);
			}
			break;
	}
}

TypeList find_last_generic_action(TypeStateActions actions, Declaration* declaration) {
	for(size_t i = actions.size; i > 0; i--) {
next:
		switch(actions.data[i - 1].type) {
			case StateActionGenerics:
				if(actions.data[i - 1].target == (void*) declaration) {
					return actions.data[i - 1].TypeList;
				}
				break;

			case StateActionCollection: {
				TypeList found = find_last_generic_action(actions.data[i - 1].TypeStateActions,
						declaration);
				if(found.size) return found;
				break;
			}
		}
	}

	return (TypeList) { 0 };
}

Type* peek_type(Type* type, TypeStateAction* action, unsigned flags) {
	if(type->compiler == (void*) &comp_Wrapper) {
		if(in_compiler_step && type->Wrapper.anchor) {
			Type* anchor = peek_type((void*) type->Wrapper.anchor, action, flags);
			return type == anchor ? type : anchor;
		}

		if(type->Wrapper.ref) {
			if(type->Wrapper.action.type) {
				if(type_state_action(type->Wrapper.action, flags)) {
					*action = type->Wrapper.action;
				}
			}

			return type->Wrapper.variable
				? type->Wrapper.ref->Declaration.const_value
				: (void*) type->Wrapper.ref;
		}

		return type;
	}

	if(type->compiler == (void*) &comp_GenericType) {
		return last(type->GenericType.declaration->generics.stack)
			.data[type->GenericType.index];
	}

	return type;
}

OpenedType open_type_wa(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
		void* accumulator, unsigned flags) {
	OpenedType opened_type = { 0 };
	if(!type) return opened_type;
	TypeStateAction action = { 0 };

	while((opened_type.type = peek_type(type, &action, flags)) != type) {
		type = opened_type.type;
		if(acceptor) acceptor(type, follower, accumulator);

		if(action.type) {
			push(&opened_type.actions, action);
			action.type = 0;
		}
	}

	return opened_type;
}

void close_type(TypeStateActions actions, unsigned flags) {
	for(size_t i = actions.size; i > 0; i--) {
		undo_type_state_action(actions.data[i - 1], flags);
	}
	free(actions.data);
}

Type* make_type_standalone(Type* type) {
	TypeStateActions actions = { 0 };
	if(!global_state_actions.size) return type;

	for(size_t i = 0; i < global_state_actions.size; i++) {
		push(&actions, global_state_actions.data[i]);
	}

	return new_type((Type) { .Wrapper = {
			.compiler = (void*) &comp_Wrapper,
			.flags = type->flags,
			.trace = type->trace,
			.action = { StateActionCollection, .TypeStateActions = actions },
			.ref = (void*) type,
	}});
}

enum {
	TraverseIntermeditate 	= 1 << 0, 
	TraverseGenerics 		= 1 << 1,
};

int traverse_type(Type*, Type*, int (*)(Type*, Type*, void*), void*, unsigned);

static inline int traverse_generics(Declaration* declaration,
		int (*acceptor)(Type*, Type*, void*), void* accumulator, unsigned flags) {
	if(!(flags & TraverseGenerics) || !declaration->generics.stack.size) return 0;
	
	const TypeList generics = last(declaration->generics.stack);
	for(size_t i = 0; i < generics.size; i++) {
		const int result = traverse_type(generics.data[i], NULL, acceptor, accumulator,
				flags);
		if(result) return result;
	}

	return 0;
}

int traverse_type(Type* type, Type* follower, int (*acceptor)(Type*, Type*, void*),
		void* accumulator, unsigned flags) {
	const OpenedType ofollower = open_type(follower, flags
			& (ActionKeepGlobalState | ActionNoChildCompilation));
	const OpenedType otype = open_type_wa(type, follower, (flags & TraverseIntermeditate)
			? acceptor : 0, accumulator, flags 
			& (ActionKeepGlobalState | ActionNoChildCompilation));

	int result = 0, roffset = 0;
	if(!(flags & TraverseIntermeditate)) {
		roffset = !!(result = acceptor(otype.type, ofollower.type, accumulator));
	}

	if(result);
	else if(follower && otype.type->compiler != ofollower.type->compiler) {
		result = 1;
	} else if(otype.type->compiler == (void*) &comp_PointerType) {
		result = traverse_type(otype.type->PointerType.base, ofollower.type
				? ofollower.type->PointerType.base : NULL, acceptor, accumulator, flags);
	} else if(otype.type->compiler == (void*) &comp_StructType) {
		if(ofollower.type == otype.type && otype.type->StructType.parent->generics.stack.size) {
			TypeList a = find_last_generic_action(otype.actions,
					(void*) otype.type->StructType.parent);
			TypeList b = find_last_generic_action(ofollower.actions,
					(void*) otype.type->StructType.parent);

			// printf("things: %zu %zu\n", a.size, b.size);
			if(!a.size) a = otype.type->StructType.parent->generics.stack.data[0];
			if(!b.size) b = otype.type->StructType.parent->generics.stack.data[0];

			for(size_t i = 0; i < a.size; i++) {
				if((result = traverse_type(a.data[i], b.data[i], acceptor,
								accumulator, flags)))
					break;
			}
		} else {
			result = traverse_generics((void*) otype.type->StructType.parent, acceptor,
					accumulator, flags);
		}

		if(result);
		else if(ofollower.type && ofollower.type->StructType.fields.size
				!= otype.type->StructType.fields.size) {
			result = 1;
		} else for(size_t i = 0; !result && i < otype.type->StructType.fields.size; i++) {
			result = traverse_type(otype.type->StructType.fields.data[i]->type,
					ofollower.type ? ofollower.type->StructType.fields.data[i]->type: 0,
					acceptor, accumulator, flags);
		}
	} else if(otype.type->compiler == (void*) &comp_FunctionType) {
		result = traverse_generics((void*) otype.type->FunctionType.declaration,
				acceptor, accumulator, flags);
	}

	close_type(otype.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation));
	close_type(ofollower.actions, flags & (ActionKeepGlobalState | ActionNoChildCompilation));
	return result - roffset;
}

enum { StringifyAlphaNum = 1 << 0 };

typedef struct {
	str* string;
	unsigned flags;
} StringifyAccumulator;

int stringify_acceptor(Type* type, Type* _, StringifyAccumulator* accumulator) {
	if(type->compiler == (void*) &comp_Wrapper) {
		strf(accumulator->string, type->flags & tfNumeric
				? accumulator->flags & StringifyAlphaNum ? "number" : "~number"
				: "auto");
	} else if(type->compiler == (void*) &comp_External) {
		strf(accumulator->string, "%.*s", (int) type->External.data.size,
				type->External.data.data);
	} else if(type->compiler == (void*) &comp_PointerType) {
		strf(accumulator->string, accumulator->flags & StringifyAlphaNum
				? "ptrto_" : "&");
	} else if(type->compiler == (void*) &comp_StructType) {
		strf(accumulator->string, accumulator->flags & StringifyAlphaNum
				? "struct_%.*s" : "struct %.*s",
				(int) type->StructType.parent->identifier->base.size,
				type->StructType.parent->identifier->base.data);
		return 1;
	} else {
		strf(accumulator->string, accumulator->flags & StringifyAlphaNum
				? "UNKNOWN" : "~unknown");
	}
	return 0;
}

void stringify_type(Type* type, str* string, unsigned flags) {
	traverse_type(type, NULL, (void*) &stringify_acceptor,
			&(StringifyAccumulator) { string, flags },
			ActionKeepGlobalState | ActionNoChildCompilation);
}

enum {
	TestMismatch = 2,
	TestCircular,

	ClashPassive = 1 << 0,
};

typedef struct {
	Trace trace;
	Messages* messages;
	unsigned flags;
} ClashAccumulator;

int circular_acceptor(Type* type, Type* _, Type* compare) {
	return 2 * (type == compare);
}

int assign_wrapper(Wrapper* wrapper, Type* follower, ClashAccumulator* accumulator) {
	printf("assigning:\t \33[3%dm%-24.*s \33[3%dm%.*s\33[0m\n",
			(int) ((size_t) wrapper->trace.slice.data / 16) % 6 + 1,
			(int) wrapper->trace.slice.size, wrapper->trace.slice.data,
			(int) ((size_t) follower->trace.slice.data / 16) % 6 + 1,
			(int) follower->trace.slice.size, follower->trace.slice.data);

	if(wrapper->flags & tfNumeric && !(follower->flags & tfNumeric)
			&& follower->compiler != (void*) &comp_Wrapper) return TestMismatch;

	if((void*) wrapper == follower) return 1;
	if(
			traverse_type((void*) wrapper, NULL, (void*) &circular_acceptor, follower,
				TraverseGenerics & TraverseIntermeditate) ||
			traverse_type((void*) follower, NULL, (void*) &circular_acceptor, wrapper,
				TraverseGenerics & TraverseIntermeditate)
	  ) return TestCircular;

	if(wrapper->compare) {
		int result = clash_types(wrapper->compare, follower, accumulator->trace,
				accumulator->messages, ClashPassive | accumulator->flags);
		if(result) return result + 1;
	}

	if(!(accumulator->flags & ClashPassive)) {
		if(follower->compiler == (void*) &comp_Wrapper && follower->Wrapper.anchor) {
			// TODO: open wrapper->compare
			if(traverse_type(follower, NULL, (void*) &circular_acceptor,
						(void*) wrapper->compare, TraverseGenerics & TraverseIntermeditate)) {
				wrapper->anchor = follower->Wrapper.anchor;
				// wrapper->anchor = (void*) new_type(*(Type*)(void*) follower->Wrapper.anchor);
				return 1;
			}

			const OpenedType anchor = open_type((void*) follower->Wrapper.anchor, 0);
			wrapper->ref = (void*) make_type_standalone(anchor.type);
			close_type(anchor.actions, 0);
		} else {
			wrapper->ref = (void*) make_type_standalone(follower);
		}

		if(wrapper->flags & tfNumeric && !(follower->flags & tfNumeric)) {
			follower->flags = wrapper->flags;
		} else {
			wrapper->flags = follower->flags;
		}

		if(follower->compiler == (void*) &comp_Wrapper) {
			if(wrapper->compare) follower->Wrapper.compare = wrapper->compare;
			if(wrapper->anchor) follower->Wrapper.anchor = wrapper->anchor;
		}
	}

	return 1;
}

int clash_acceptor(Type* type, Type* follower, ClashAccumulator* accumulator) {
	printf("\33[90mclash:\t\t %-24.*s %.*s\33[0m\n",
			(int) type->trace.slice.size, type->trace.slice.data,
			(int) follower->trace.slice.size, follower->trace.slice.data);

	if(type->compiler == (void*) &comp_Wrapper &&
			!(type->Wrapper.anchor && follower->compiler == (void*) &comp_Wrapper)) {
		return assign_wrapper((void*) type, follower, accumulator);
	} else if(follower->compiler == (void*) &comp_Wrapper) {
		return assign_wrapper((void*) follower, type, accumulator);
	} else if(type->compiler != follower->compiler);

	else if(type->compiler == (void*) &comp_External) {
		if(streq(type->External.data, follower->External.data)) return 1;
	} else return 0;

	if(type->flags & follower->flags & tfNumeric) return 1;
	return TestMismatch;
}

int clash_types(Type* a, Type* b, Trace trace, Messages* messages, unsigned flags) {
	ClashAccumulator accumulator = { trace, messages, flags };
	int result = traverse_type(a, b, (void*) &clash_acceptor, &accumulator, 0);

	if(result && !(flags & ClashPassive)) {
		str message = strf(0, "type mismatch between '\33[35m");
		stringify_type(a, &message, 0);

		strf(&message, "\33[0m' and '\33[35m");
		stringify_type(b, &message, 0);

		strf(&message, result + 1 == TestCircular
				? "\33[0m' (types are circularly referencing eachother)"
				: "\33[0m'");

		push(messages, Err(trace, message));
	}

	return result;
}
