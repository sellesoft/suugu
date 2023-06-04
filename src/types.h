﻿/* Index:
@vec2f64
@element
@term
@expression
*/

#pragma once
#ifndef SUUGU_TYPES_H
#define SUUGU_TYPES_H

#include "kigu/common.h"
#include "core/memory.h"
#include "external/stb/stb_ds.h"

struct Expression;

// represents a unit, which may or may not be composed of other units
struct Unit{
	Unit* unit; //array of units that this unit may be made of, 0 if base unit
	str8 id; 	   // the unique identifier of this unit 
	str8 quantity; // the physical quantity this unit represents such as length, mass, etc.
	str8 symbols;  // symbols this unit may use
	str8 description;
};

struct Variable{
	Expression* expr; //TODO maybe let variable terms have expression children rather than a tree disconnect
	str8 name;
	Unit* unit;
	str8* symbols; // a list of symbols that this variable may take on. if a symbol conflicts with another, we will try to use a different one to avoid conflicts
	b32 right_of_equals;
};

struct Function{
	str8  text;
	void* ptr;
	s32   args;
};
typedef f64(*Function1Arg)(f64 a);




////workspace: region of the canvas in which expressions are able to interact together  
//struct Expression;
//struct Workspace{
//Element element;
//str8 name;
//arrayT<Expression*> expressions = arrayT<Expression*>(deshi_allocator);
//};
//#define ElementToWorkspace(elem_ptr) ((Workspace*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(Workspace, element))))

////graph: graphing grid with a local camera in which equations can be drawn
// struct GraphElement{
// 	Element element;
// 	uiGraphCartesian* cartesian_graph;
// };
//TODO(sushi) remove this and usage of it since we can just use normal C casting
#define ElementToGraphElement(elem_ptr) ((GraphElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(GraphElement, element))))

//struct TextElement{
//Element element;
//str8 text;
//Font* font;
//f32 font_height;
//vec2 scale;
//f32 rotation;
//};
//#define ElementToTextElement(elem_ptr) ((TextElement*)((u8*)(elem_ptr) - (upt)(OffsetOfMember(TextElement, element))))

/////////////////
//// @pencil ////
/////////////////
struct PencilStroke{
	f32   size;
	color color;
	vec2* pencil_points; // kigu array
};

////////////////
//// @tools ////
////////////////
enum CanvasTool : u32{
	CanvasTool_Navigation,
	CanvasTool_Context,
	CanvasTool_Expression,
	CanvasTool_Pencil,
};
local const char* canvas_tool_strings[] = {
	"Navigation", "Context", "Expression", "Pencil",
};

////////////////
//// @binds ////
////////////////
enum CanvasBind_{ //TODO ideally support multiple keybinds per action
	//[GLOBAL] SetTool
	CanvasBind_SetTool_Navigation = Key_ESCAPE  | InputMod_Any,
	CanvasBind_SetTool_Context    = Mouse_RIGHT | InputMod_AnyCtrl,
	CanvasBind_SetTool_Expression = Key_E       | InputMod_AnyCtrl, //NOTE temp making this CTRL+E for simplicity
	CanvasBind_SetTool_Pencil     = Key_P       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Graph      = Key_G       | InputMod_AnyCtrl,
	CanvasBind_SetTool_Previous   = Mouse_4     | InputMod_None,
	
	//[GLOBAL] Camera 
	CanvasBind_Camera_Pan = Mouse_MIDDLE | InputMod_None,
	
	//[LOCAL]  Navigation 
	CanvasBind_Navigation_Pan       = Mouse_LEFT  | InputMod_Any,
	CanvasBind_Navigation_ResetPos  = Key_NP0     | InputMod_None,
	CanvasBind_Navigation_ResetZoom = Key_NP0     | InputMod_None,
	
	//[LOCAL]  Expression
	CanvasBind_Expression_Select                = Mouse_LEFT    | InputMod_None,
	CanvasBind_Expression_Create                = Mouse_RIGHT   | InputMod_None,
	CanvasBind_Expression_CursorLeft            = Key_LEFT      | InputMod_None,
	CanvasBind_Expression_CursorRight           = Key_RIGHT     | InputMod_None,
	CanvasBind_Expression_CursorWordLeft        = Key_LEFT      | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorWordRight       = Key_RIGHT     | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorUp              = Key_UP        | InputMod_None,
	CanvasBind_Expression_CursorDown            = Key_DOWN      | InputMod_None,
	CanvasBind_Expression_CursorHome            = Key_HOME      | InputMod_None,
	CanvasBind_Expression_CursorEnd             = Key_END       | InputMod_None,
	CanvasBind_Expression_CursorDeleteLeft      = Key_BACKSPACE | InputMod_None,
	CanvasBind_Expression_CursorDeleteRight     = Key_DELETE    | InputMod_None,
	CanvasBind_Expression_CursorDeleteWordLeft  = Key_BACKSPACE | InputMod_AnyCtrl,
	CanvasBind_Expression_CursorDeleteWordRight = Key_DELETE    | InputMod_AnyCtrl,
	
	//[LOCAL]  Pencil
	CanvasBind_Pencil_Stroke             = Mouse_LEFT       | InputMod_Any,
	CanvasBind_Pencil_DeletePrevious     = Key_Z            | InputMod_AnyCtrl,
	CanvasBind_Pencil_DetailIncrementBy1 = Key_EQUALS       | InputMod_None,
	CanvasBind_Pencil_DetailIncrementBy5 = Key_EQUALS       | InputMod_AnyShift,
	CanvasBind_Pencil_DetailDecrementBy1 = Key_MINUS        | InputMod_None,
	CanvasBind_Pencil_DetailDecrementBy5 = Key_MINUS        | InputMod_AnyShift,
}; typedef KeyCode CanvasBind;




//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @term
enum TermType : u32 { //TermType_{
	TermType_Expression,
	TermType_Operator,
	TermType_Literal,
	TermType_Variable,
	TermType_FunctionCall,
	TermType_Logarithm,
}; //typedef Type TermType;

enum TermFlags_{
	TermFlag_NONE = 0,
	
	//// operator argument flags //// //NOTE these flags are mainly used to determine empty slots on operators
	//TODO(delle) explain these better, i forgot what they do and why, seems to be mostly for rendering?
	TermFlag_OpArgLeft   = (1 << 0),
	TermFlag_OpArgRight  = (1 << 1),
	TermFlag_OpArgTop    = (1 << 2),
	TermFlag_OpArgBottom = (1 << 3),
	
	TermFlag_LeftParenHasMatchingRightParen = (1 << 4),
	TermFlag_DanglingClosingParenToRight    = (1 << 5),
	//TermFlag_DrawParenthesesAsBlock         = (1 << 6), //TODO symbolab style input boxes (test with parentheses and exponents)
}; typedef Flags TermFlags;
#define OPARG_MASK (TermFlag_OpArgLeft | TermFlag_OpArgRight | TermFlag_OpArgTop | TermFlag_OpArgBottom)
#define RemoveOpArgs(var) RemoveFlag(var, OPARG_MASK)
#define ReplaceOpArgs(var, new_flags) ((var) = (((var) & ~OPARG_MASK) | new_flags))

//operator: symbol that represents an operation on one or many terms
//NOTE in order of precedence, so the higher value it is (lower sequentially), the lower the precedence
//NOTE these are operation-based operators, not token-based operators
//TODO try to find a way to store the number of arguments
enum OpType : u32{//temp c++ enum thing for debugging
	OpType_NULL = 0,
	
	OpPrecedence_1  = (1 << 8),
	OpType_Parentheses, //TODO maybe just make this a term type since it doesnt behave like an operator?
	//OpType_AbsoluteValue,
	//OpType_Root,
	//OpType_Derivative,
	//OpType_Integral,
	//OpType_Limit,
	//OpType_Sum,
	//OpType_PartialDerivative
	
	OpPrecedence_2  = (1 << 9),
	OpType_Exponential,
	
	OpPrecedence_3  = (1 << 10),
	OpType_Negation,
	//OpType_BitwiseNOT,
	//OpType_LogicalNOT,
	
	OpPrecedence_4  = (1 << 11),
	OpType_ImplicitMultiplication, //5x
	OpType_ExplicitMultiplication, //5*x
	OpType_Division,
	OpType_Modulo,
	
	OpPrecedence_5  = (1 << 12),
	OpType_Addition,
	OpType_Subtraction,
	
	OpPrecedence_6  = (1 << 13),
	//OpType_ArithmaticShiftLeft,
	//OpType_ArithmaticShiftRight,
	//OpType_LogicalShiftLeft,
	//OpType_LogicalShiftRight,
	//OpType_CircularShiftLeft,
	//OpType_CircularShiftRight,
	
	OpPrecedence_7  = (1 << 14),
	//OpType_LessThan,
	//OpType_LessThanEqual,
	//OpType_GreaterThan,
	//OpType_GreaterThanEqual,
	
	OpPrecedence_8  = (1 << 15),
	//OpType_BooleanEquals,
	//OpType_BooleanNotEquals,
	
	OpPrecedence_9  = (1 << 16),
	//OpType_BitwiseAND,
	
	OpPrecedence_10 = (1 << 17),
	//OpType_BitwiseXOR,
	
	OpPrecedence_11 = (1 << 18),
	//OpType_BitwiseOR,
	
	OpPrecedence_12 = (1 << 19),
	//OpType_LogicalAND,
	
	OpPrecedence_13 = (1 << 20),
	//OpType_LogicalOR,
	
	OpPrecedence_14 = (1 << 21),
	OpType_ExpressionEquals, //NOTE this should be one of the lowest precedence operators
}; //typedef Type OpType;
#define OPPRECEDENCE_MASK 0xFFFFFF00
//NOTE these are inverted b/c the precedence flags get larger as they decrease in precedence
#define OpIsGreater(op1,op2)      (((op1)->op_type & OPPRECEDENCE_MASK) <  ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsGreaterEqual(op1,op2) (((op1)->op_type & OPPRECEDENCE_MASK) <= ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsLesser(op1,op2)       (((op1)->op_type & OPPRECEDENCE_MASK) >  ((op2)->op_type & OPPRECEDENCE_MASK))
#define OpIsLesserEqual(op1,op2)  (((op1)->op_type & OPPRECEDENCE_MASK) >= ((op2)->op_type & OPPRECEDENCE_MASK))

static str8 OpTypeStrs(u32 type){
	switch(type){
		case OpType_NULL:                   return str8l("OpType_NULL");
		case OpPrecedence_1:                return str8l("OpPrecedence_1");
		case OpType_Parentheses:            return str8l("OpType_Parentheses");
		case OpPrecedence_2:                return str8l("OpPrecedence_2");
		case OpType_Exponential:            return str8l("OpType_Exponential");
		case OpPrecedence_3:                return str8l("OpPrecedence_3");
		case OpType_Negation:               return str8l("OpType_Negation");
		case OpPrecedence_4:                return str8l("OpPrecedence_4");
		case OpType_ImplicitMultiplication: return str8l("OpType_ImplicitMultiplication");
		case OpType_ExplicitMultiplication: return str8l("OpType_ExplicitMultiplication");
		case OpType_Division:               return str8l("OpType_Division");
		case OpType_Modulo:                 return str8l("OpType_Modulo");
		case OpPrecedence_5:                return str8l("OpPrecedence_5");
		case OpType_Addition:               return str8l("OpType_Addition");
		case OpType_Subtraction:            return str8l("OpType_Subtraction");
		case OpPrecedence_6:                return str8l("OpPrecedence_6");
		case OpPrecedence_7:                return str8l("OpPrecedence_7");
		case OpPrecedence_8:                return str8l("OpPrecedence_8");
		case OpPrecedence_9:                return str8l("OpPrecedence_9");
		case OpPrecedence_10:               return str8l("OpPrecedence_10");
		case OpPrecedence_11:               return str8l("OpPrecedence_11");
		case OpPrecedence_12:               return str8l("OpPrecedence_12");
		case OpPrecedence_13:               return str8l("OpPrecedence_13");
		case OpPrecedence_14:               return str8l("OpPrecedence_14");
		default:                            return str8l("invalid term type");
	}
}

//term: generic base thing (literal, operator, variable, function call, etc)
struct Term{
	TermType  type;
	TermFlags flags;
	str8 raw;
	
	union{
		OpType op_type;
		f64 lit_value;
		Function* func;
		f64 log_base;
		Variable var;
	};
	
	//syntax tree
	Term* prev;
	Term* next;
	Term* parent;
	Term* first_child;
	Term* last_child;
	u32   child_count;
	
	//linear
	Term* left;
	Term* right;
	Term* outside;
	Term* first_inside;
	Term* last_inside;
};

global void ast_insert_after(Term* target, Term* term){
	if(target->next) target->next->prev = term;
	term->next = target->next;
	term->prev = target;
	target->next = term;
}

global void ast_insert_before(Term* target, Term* term){
	if(target->prev) target->prev->next = term;
	term->prev = target->prev;
	term->next = target;
	target->prev = term;
}

global void ast_remove_horizontally(Term* term){
	if(term->next) term->next->prev = term->prev;
	if(term->prev) term->prev->next = term->next;
	term->next = term->prev = 0;
}

global void ast_insert_last(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		ast_insert_after(parent->last_child, child);
		parent->last_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global void ast_insert_first(Term* parent, Term* child){
	child->parent = parent;
	if(parent->first_child){
		ast_insert_before(parent->first_child, child);
		parent->first_child = child;
	}else{
		parent->first_child = child;
		parent->last_child  = child;
	}
	parent->child_count++;
}

global void ast_remove_from_parent(Term* term){
	if(term->parent == 0) return;
	if(term->parent->child_count > 1){
		if(term == term->parent->first_child) term->parent->first_child = term->next;
		if(term == term->parent->last_child)  term->parent->last_child  = term->prev;
	}else{
		Assert(term == term->parent->first_child && term == term->parent->last_child, "if term is the only child term, it should be both the first and last child terms");
		term->parent->first_child = 0;
		term->parent->last_child  = 0;
	}
	term->parent->child_count--;
}

global void ast_change_parent_insert_last(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	ast_remove_from_parent(term);
	ast_remove_horizontally(term);
	ast_insert_last(new_parent, term);
}

global void ast_change_parent_insert_first(Term* new_parent, Term* term){
	if(new_parent == term->parent) return;
	ast_remove_from_parent(term);
	ast_remove_horizontally(term);
	ast_insert_first(new_parent, term);
}

global void linear_insert_left(Term* target, Term* term){
	if(target->left) target->left->right = term;
	term->right = target;
	term->left  = target->left;
	target->left = term;
	if(target->outside){
		term->outside = target->outside;
		if(target == target->outside->first_inside){
			target->outside->first_inside = term;
		}
	}
}

global void linear_insert_right(Term* target, Term* term){
	if(target->right) target->right->left = term;
	term->left  = target;
	term->right = target->right;
	target->right = term;
	if(target->outside){
		term->outside = target->outside;
		if(target == target->outside->last_inside){
			target->outside->last_inside = term;
		}
	}
}

global void linear_remove(Term* term){
	if(term->right) term->right->left = term->left;
	if(term->left)  term->left->right = term->right;
	if(term->outside){
		if(term == term->outside->first_inside) term->outside->first_inside = term->right;
		if(term == term->outside->last_inside) term->outside->last_inside = term->left;
	}
}


//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @expression
//expression: collection of terms in the form of a syntax tree
struct Expression{
	Node node;
	//Workspace* workspace;
	
	str8_builder raw;
	
	b32 changed;
	Term term;
	arrayT<Term> terms; //NOTE temporary until expression arena
	Term* equals;
	Term* rightmost;
	
	u32 raw_cursor_start;
	Term* term_cursor_start;
	b32 right_paren_cursor;
	
	b32 valid;
	u32 unknown_vars;
	f64 solution;
};

//~////////////////////////////////////////////////////////////////////////////////////////////////
//// @element
enum CoordinateSpace : u32{
	//CoordinateSpace_World,
	//CoordinateSpace_Screen,
	//CoordinateSpace_Workspace, //TODO maybe add local to workspace
};

enum ElementType : u32{
	ElementType_NULL,
	ElementType_Expression,
	//ElementType_Workspace,
	ElementType_Graph,
	//ElementType_Text,
};

// element: anything with position, size, coordinate space, and display info
// tagged union/variant
external struct Element{
	union{
		struct{f32 x,y,z;};
		vec3 pos;
	};
	union{
		struct{f32 width,height,depth;};
		vec3 size;
	};
	//CoordinateSpace space;
	ElementType type;
	uiItem* item; // handle to the uiItem representing this Element

	union{
		Expression expression;
		uiGraphCartesian cartesian_graph;
	};
};

// NOTE(sushi) this compiles on clang, need to know if it compiles on MSVC as well
const uiStyle element_default_style = {
	.positioning = pos_relative,
	.sizing = size_auto,
	.background_color = Color_Black,
	.border_style = border_solid,
	.border_width = 1,
	.text_wrap = text_wrap_none,
};

struct{
	Arena* elements;
	Node inactive_elements;
	Arena* terms;
	Node inactive_terms;
}arenas;

void memory_init() {
	arenas.elements = memory_create_arena(500*sizeof(Element));
	arenas.terms = memory_create_arena(5000*sizeof(Term));
}

global Element* make_element(){
	Element* element;
	if(arenas.inactive_elements.next){
		element = (Element*)arenas.inactive_elements.next;
		NodeRemove(arenas.inactive_elements.next);
		ZeroMemory(element, sizeof(Element));
	}else{
		element = memory_allocT(Element);
	}
	return element;
}

global Term* make_term(Expression* expr, str8 raw, TermType type){
	expr->terms.add(Term{}); //TODO expression arena
	Term* result = &expr->terms[expr->terms.count-1];
	result->type = type;
	result->raw  = raw;
	return result;
}


#endif //SUUGU_TYPES_H
