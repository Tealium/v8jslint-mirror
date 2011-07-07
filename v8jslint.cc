#include <v8.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Application exit codes.
enum {
	ERR_NO_ERROR = 0,
	ERR_COMPILATION_FAILURE,
	ERR_FILE_MISSING,
	ERR_FILE_EMPTY,
	ERR_CONTEXT_FAILURE
};

// Extracts a C string from a V8 Utf8Value.
static const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<NULL>";
}

// The callback invoked for the JavaScript 'print' function.
static v8::Handle<v8::Value> Print(const v8::Arguments& args) {
	if (0 <	args.Length())
	{
		v8::HandleScope handle_scope;
		v8::String::Utf8Value str(args[0]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);

		for (int i = 1; i < args.Length(); i++) {
			v8::HandleScope handle_scope;
			v8::String::Utf8Value str(args[i]);
			const char* cstr = ToCString(str);
			printf(" %s", cstr);
		}
	}
	printf("\n");
	fflush(stdout);
	return v8::Undefined();
}

// Reads a file into a v8 string.
static v8::Handle<v8::String> ReadFile(const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL)
		return v8::Handle<v8::String>();

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (int i = 0; i < size;) {
		int read = fread(&chars[i], 1, size - i, file);
		i += read;
	}
	fclose(file);

	v8::Handle<v8::String> result = v8::String::New(chars, size);
	delete[] chars;
	return result;
}

static void ReportException(v8::TryCatch* try_catch) {
	v8::HandleScope 		handle_scope;
	v8::String::Utf8Value 	exception(try_catch->Exception());
	v8::Handle<v8::Message>	message = try_catch->Message();

	if (message.IsEmpty()) {
		// No extra information so just print the exception.
		printf("%s\n", ToCString(exception));
	} else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		printf("%s:%i: %s\n", ToCString(filename), message->GetLineNumber(), ToCString(exception));

		// Print line of source code.
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		printf("%s\n", ToCString(sourceline));

		// Print error indicator.
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++) {
			printf(" ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++) {
			printf("^");
		}
		printf("\n");

		// Print stacktrace.
		v8::String::Utf8Value stack_trace(try_catch->StackTrace());
		if (stack_trace.length() > 0) {
			printf("%s\n", ToCString(stack_trace));
		}
	}
}

// Executes a string within the current v8 context.
static v8::Handle<v8::Value> ExecuteString(v8::Handle<v8::String> source,
                          	               v8::Handle<v8::Value> name) {
    v8::TryCatch try_catch;

    v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
    if (script.IsEmpty()) {
        // Print compilation errors.
        ReportException(&try_catch);
        return v8::Undefined();
    }

    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
        assert(try_catch.HasCaught());
        // Print execution errors.
        ReportException(&try_catch);
        return v8::Undefined();
    }

    // Return true if the javascript did not return a value
    if (result->IsUndefined()) {
        return v8::Boolean::New(true);
	}
    return result;
}

// Run JSLint.
static int RunJSLint(v8::Handle<v8::Context> context) {
	// Enter the execution environment before evaluating any code.
	v8::Context::Scope 		context_scope(context);
	v8::HandleScope 		handle_scope;
	v8::Handle<v8::Value>	result;
	// Include the wrapped JSLINT source code
    // reswrap spits the source code out as unsigned char so cast
#include "jslint.cc"
	// Include the wrapped source code to run and report
    // reswrap spits the source code out as unsigned char so cast
#include "lint.cc"

	// Execute the JSLINT source
	result = ExecuteString(v8::String::New((const char*)jslint, sizeof(jslint)/sizeof(jslint[0])), 
                           v8::String::New("JSLint"));
	if (result->IsUndefined()) {
		return ERR_COMPILATION_FAILURE;
	}

	// Execute the JSLINT run and report source
	result = ExecuteString(v8::String::New((const char*)lint, sizeof(lint)/sizeof(lint[0])), 
                           v8::String::New("JSLint run and report"));
	if (result->IsUndefined()) {
		return ERR_COMPILATION_FAILURE;
	}

	return ERR_NO_ERROR;
}

// Print Version.
static void Version(const char* exe)
{
     printf("%s Copyright (c) 2011 Steven Reid\n", exe);
}

// Print usage.
static void Usage(const char* exe)
{
     printf("Usage:  %s [options] [directives] file\n\n", exe);
     printf("Options\n");
     printf(" -(v|version)  : Show version.\n\n");
     printf("Directives. See http://www.JSLint.com/lint.html\n");
     printf(" --adsafe      : true if ADsafe rules should be enforced. See http://www.ADsafe.org/\n");
     printf(" --bitwise     : true if bitwise operators should be allowed.\n");
     printf(" --browser     : true if the standard browser globals should be predefined.\n");
     printf(" --cap         : true if uppercase HTML should be allowed.\n");
     printf(" --confusion   : true if types can be used inconsistently.\n");
     printf(" --continue    : true if the continue statement should be allowed.\n");
     printf(" --css         : true if CSS workarounds should be tolerated. \n");
     printf(" --debug       : true if debugger statements should be allowed. Set this option to false before going into production.\n");
     printf(" --devel       : true if browser globals that are useful in development should be predefined.\n");
     printf(" --eqeq        : true if the == and != operators should be tolerated.\n");
     printf(" --es5         : true if ES5 syntax should be allowed.\n");
     printf(" --evil        : true if eval should be allowed.\n");
     printf(" --forin       : true if unfiltered for in statements should be allowed. \n");
     printf(" --fragment    : true if HTML fragments should be allowed.\n");
     printf(" --indent      : The number of spaces used for indentation (default is 4). If 0, then no indentation checking takes place.\n");
     printf(" --maxerr      : The maximum number of warnings reported. (default is 50)\n");
     printf(" --maxlen      : The maximum number of characters in a line.\n");
     printf(" --newcap      : true if Initial Caps with constructor functions is optional.\n");
     printf(" --node        : true if Node.js globals should be predefined. \n");
     printf(" --nomen       : true if names should not be checked for initial or trailing underbars.\n");
     printf(" --on          : true if HTML event handlers should be allowed.\n");
     printf(" --passfail    : true if the scan should stop on first error.\n");
     printf(" --plusplus    : true if ++ and -- should be allowed.\n");
//	 Unlisted directive at https://github.com/douglascrockford/JSLint/blob/master/jslint.js
//   printf(" --predef      : An array of strings, the names of predefined global variables, or an object whose keys are global variable names, and whose values are booleans that determine if each variable is assignable.\n");
     printf(" --regexp      : true if . and [^...] should be allowed in RegExp literals.\n");
     printf(" --rhino       : true if the Rhino environment globals should be predefined. \n");
     printf(" --safe        : true if the safe subset rules are enforced. These rules are used by ADsafe. It enforces the safe subset rules but not the widget structure rules.\n");
     printf(" --sloppy      : true if the ES5 'use strict'; pragma is not required.\n");
     printf(" --sub         : true if subscript notation may be used for expressions better expressed in dot notation.\n");
     printf(" --undef       : true if variables can be declared out of order. \n");
     printf(" --unparam     : true if unused parameters should be tolerated.\n");
     printf(" --vars        : true if multiple var statement per function should be allowed. \n");
     printf(" --white       : true if strict whitespace rules should be ignored.\n");
     printf(" --widget      : true if the Yahoo Widgets globals should be predefined.\n");
     printf(" --windows     : true if the Windows globals should be predefined.\n");
}

static int RunMain(int argc, char* argv[]) {
	v8::HandleScope 				handle_scope;
	const char 						*filename = NULL;
	v8::Handle<v8::ObjectTemplate>	option = v8::ObjectTemplate::New();

    // set the default directives. See http://www.jslint.com/
    option->Set(v8::String::New("devel"), v8::Boolean::New(true));
    option->Set(v8::String::New("browser"), v8::Boolean::New(true));
    option->Set(v8::String::New("es5"), v8::Boolean::New(true));
    option->Set(v8::String::New("evil"), v8::Boolean::New(false));
    option->Set(v8::String::New("maxerr"), v8::Number::New(50));
    option->Set(v8::String::New("indent"), v8::Number::New(4));

	for (int i = 1; i < argc; ++i) {
        // A jslint directive is specified by double dash
		if (0 == strncmp(argv[i], "--", 2)) {
             // Search for the "="
             char *ch = strchr(argv[i]+2, '=');
             if (NULL == ch) {
                 // default the directive property value to 'false'
                 option->Set(v8::String::New(argv[i]+2), v8::String::New("false"));
             } else {
                // terminate at the "=" to extract the directive
                *(argv[i] + (ch-argv[i])) = '\0';
                // set the directive property and value
                option->Set(v8::String::New(argv[i]+2), v8::String::New(ch+1));
             }
		// A v8-jslint option is specified by single dash
		} else if (0 == strncmp(argv[i], "-", 1)) {
			// version option
			if ((0 == strcmp(argv[i], "-v")) || (0 == strcmp(argv[i], "-version"))) {
				Version(argv[0]);
				return ERR_NO_ERROR;
            }
		// The file to be verified is specified without dashes
		} else if (NULL == filename) {
			filename = argv[i];
		}
	}

	// Must have a file to verify
	if (NULL == filename) {
		Usage(argv[0]);
		return ERR_FILE_MISSING;
	}

	// Read the Javascript file into the source string
	v8::Handle<v8::String> source = ReadFile(filename);
	if (source.IsEmpty()) {
		Usage(argv[0]);
		return ERR_FILE_EMPTY;
	}

	// Create a template for the global object.
	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

	// Bind the global 'print' function
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));

	// Bind the global 'source' string to be run by JSLINT
	global->Set(v8::String::New("source"), source);

	// Bind the global 'option' template to configure JSLINT
	global->Set(v8::String::New("option"), option);

	// Bind the global 'filename' string to be verified
	global->Set(v8::String::New("filename"), v8::String::New(filename));

	// Create the execution context
	v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);
	if (context.IsEmpty()) {
		return ERR_CONTEXT_FAILURE;
	}

	// Run JSLint in the execution context.
	context->Enter();
	int result = RunJSLint(context);
	context->Exit();
	context.Dispose();
	return result;
}

int main(int argc, char* argv[]) {
    v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
    int result = RunMain(argc, argv);
    v8::V8::Dispose();
    return result;
}
