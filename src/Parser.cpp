#include "parser.h"

#include "utils/map.h"
#include "core/console.h"

//master token
token curt;
array<token> tokens;

#define token_next curt = tokens.next()
#define token_last curt = tokens.prev()

#define token_peek tokens.peek(0)
#define token_look_back(i) tokens.lookback(i)

#define Expect(tok_type)\
if(curt.type == tok_type) {

#define ElseExpect(tok_type)\
} else if(curt.type == tok_type) {

#define ExpectFail(fail_code)\
} else { fail_code }

#define ParseOut(out)\
LOG(out);


#define ExpectOneOf(expect)\
if(expect.has(curt.type)) {

u32 layer = 0;
bool master_logger = true;

#define PrettyPrint(message)\
if(master_logger){ for(int i = 0; i < layer; i++)\
if(i % 2 == 0) std::cout << "|   ";\
else std::cout << "!   ";\
std::cout << TOSTRING("~", message, ":").str << std::endl;}

#define EOICheck \
if(tokens.iter > tokens.last) return;



local map<token_type, ExpressionType> binaryOps{
	{tok_Multiplication,     Expression_BinaryOpMultiply},
	{tok_Division,           Expression_BinaryOpDivision},
	{tok_Negation,           Expression_BinaryOpMinus},
	{tok_Plus,               Expression_BinaryOpPlus},
	{tok_AND,                Expression_BinaryOpAND},
	{tok_OR,                 Expression_BinaryOpOR},
	{tok_LessThan,           Expression_BinaryOpLessThan},
	{tok_GreaterThan,        Expression_BinaryOpGreaterThan},
	{tok_LessThanOrEqual,    Expression_BinaryOpLessThanOrEqual},
	{tok_GreaterThanOrEqual, Expression_BinaryOpGreaterThanOrEqual},
	{tok_Equal,              Expression_BinaryOpEqual},
	{tok_NotEqual,           Expression_BinaryOpNotEqual},
	{tok_BitAND,             Expression_BinaryOpBitAND},
	{tok_BitOR,              Expression_BinaryOpBitOR},
	{tok_BitXOR,		     ExpressionGuard_BitXOR},
	{tok_BitShiftLeft,       Expression_BinaryOpBitShiftLeft},
	{tok_BitShiftRight,      Expression_BinaryOpBitShiftRight},
	{tok_Modulo,             Expression_BinaryOpModulo},
};

local map<token_type, ExpressionType> unaryOps{
	{tok_BitwiseComplement, Expression_UnaryOpBitComp},
	{tok_LogicalNOT,        Expression_UnaryOpLogiNOT},
	{tok_Negation,          Expression_UnaryOpNegate},
};


void parse_term(array<Expression>*expressions);
void parse_expressions(array<Expression>*expressions);

void parse_factor(array<Expression>* expressions) {
	layer++;
	PrettyPrint("factor");
	EOICheck;

	switch (curt.type) {

		case tok_Literal: {
			PrettyPrint("literal ", curt.str);
			expressions->add(Expression(curt.str, Expression_Literal));
			return;
		}break;

		case tok_OpenParen: {
			PrettyPrint("OPEN (");

			expressions->add(Expression(curt.str, ExpressionGuard_Factor));
			
			token_next;
			parse_expressions(&expressions->last->expressions);
			
			Expect(tok_CloseParen)
				PrettyPrint("CLOSE )");
				return;
			ExpectFail()
		}break;

		case tok_Identifier: {
			//expressions->add(new Expression(curt.str, Expression_IdentifierRHS));
			return;
		}break;

		default: {
			ExpectOneOf(unaryOps) 
				PrettyPrint("unaop ", ExpTypeStrings[binaryOps.at(curt.type)]);

				expressions->add(Expression(curt.str, *unaryOps.at(curt.type)));
			
				token_next;
				parse_factor(&expressions->last->expressions);
				
				return;
			ExpectFail()
		}
	}

	layer--;
}

void parse_term(array<Expression>* expressions){
	layer++;
	PrettyPrint("term");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_Term));
	parse_factor(&expressions->last->expressions);

	while (
		token_peek.type == tok_Multiplication || token_peek.type == tok_Division ||
		token_peek.type == tok_Modulo) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_Term));
		parse_factor(&expressions->last->expressions);
	}

	layer--;
}

void parse_additive(array<Expression>* expressions){
	layer++;
	PrettyPrint("additive");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_Additive));
	parse_term(&expressions->last->expressions);

	while (token_peek.type == tok_Plus || token_peek.type == tok_Negation) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);
		
		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		token_next; //skip over binop token
		expressions->add(Expression(curt.str, ExpressionGuard_Additive));
		parse_term(&expressions->last->expressions);
	}

	layer--;
}


void parse_bitwise_shift(array<Expression>* expressions){
	layer++;
	PrettyPrint("bitshift");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
	parse_additive(&expressions->last->expressions);

	while (token_peek.type == tok_BitShiftRight || token_peek.type == tok_BitShiftLeft) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_BitShift));
		parse_additive(&expressions->last->expressions);
	}

	layer--;
}

void parse_relational(array<Expression>* expressions){
	layer++;
	PrettyPrint("relational");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_Relational));
	parse_bitwise_shift(&expressions->last->expressions);

	while (
		token_peek.type == tok_LessThan ||
		token_peek.type == tok_GreaterThan ||
		token_peek.type == tok_LessThanOrEqual ||
		token_peek.type == tok_GreaterThanOrEqual) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, *binaryOps.at(curt.type)));

		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_Relational));
		parse_bitwise_shift(&expressions->last->expressions);
	}

	layer--;
}

void parse_equality(array<Expression>* expressions){
	layer++;
	PrettyPrint("equality");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_Equality));
	parse_relational(&expressions->last->expressions);

	while (token_peek.type == tok_NotEqual || token_peek.type == tok_Equal) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpEqual));

		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_Equality));
		parse_relational(&expressions->last->expressions);
	}

	layer--;
}

void parse_bitwise_and(array<Expression>* expressions){
	layer++;
	PrettyPrint("bit and");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
	parse_equality(&expressions->last->expressions);

	while (token_peek.type == tok_BitAND) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitAND));

		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_BitAND));
		parse_equality(&expressions->last->expressions);
	}
	layer--;
}

void parse_bitwise_xor(array<Expression>* expressions){
	layer++;
	PrettyPrint("bit xor");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
	parse_bitwise_and(&expressions->last->expressions);

	while (token_peek.type == tok_BitXOR) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpXOR));

		token_next;
		expressions->add(Expression(curt.str, ExpressionGuard_BitXOR));
		parse_bitwise_and(&expressions->last->expressions);
	}
	layer--;
}

void parse_bitwise_or(array<Expression>* expressions) {
	layer++;
	PrettyPrint("bit or");
	EOICheck;

	expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
	parse_bitwise_xor(&expressions->last->expressions);

	while (token_peek.type == tok_BitOR) {
		token_next;
		PrettyPrint("binop ", ExpTypeStrings[binaryOps.at(curt.type)]);

		//operator expression
		expressions->add(Expression(curt.str, Expression_BinaryOpBitOR));

		token_next; 
		expressions->add(Expression(curt.str, ExpressionGuard_BitOR));
		parse_bitwise_xor(&expressions->last->expressions);
	}
	layer--;
}

void parse_expressions(array<Expression>* expressions) {
	EOICheck;
	PrettyPrint("expression preface");

	expressions->add(Expression(curt.str, ExpressionGuard_Preface));
	parse_bitwise_or(&expressions->last->expressions);
}

Statement Parser::parse(array<token> _tokens){
	tokens = _tokens;
	curt = tokens[0];
	Statement statement;

	statement.expressions = new array<Expression>;

	parse_expressions(statement.expressions);
	
	return statement;
}


