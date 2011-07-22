#include <v8.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Application exit codes.
enum {
    ERR_NO_ERROR = 0,
    ERR_NO_SOURCE,
    ERR_CONTEXT_FAILURE,
    ERR_COMPILATION_FAILURE
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

// Reads standard input into a v8 string.
static v8::Handle<v8::String> ReadInput() {
    char chars[256];

    v8::Handle<v8::String> result = v8::String::Empty();
    for (int read = 0; 0 < (read = fread(&chars[0], 1, sizeof(chars)/sizeof(chars[0]), stdin));) {
        v8::Handle<v8::String>  tmp = v8::String::New(chars, read);
        result = v8::String::Concat(result, tmp);
    }
    
    return result;
}

static void ReportException(v8::TryCatch* try_catch) {
    v8::HandleScope         handle_scope;
    v8::String::Utf8Value   exception(try_catch->Exception());
    v8::Handle<v8::Message> message = try_catch->Message();

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
    v8::Context::Scope      context_scope(context);
    v8::HandleScope         handle_scope;
    v8::Handle<v8::Value>   result;
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
     printf("Directives\n");
     printf("See http://www.JSLint.com/lint.html for the directives.\n");
}

static int RunMain(int argc, char* argv[]) {
    v8::HandleScope                 handle_scope;
    const char                      *file = NULL;
    v8::Handle<v8::ObjectTemplate>  option = v8::ObjectTemplate::New();

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
        } else if (NULL == file) {
            file = argv[i];
        }
    }

    // The Javascript filename to be linted
    v8::Handle<v8::String> filename = v8::String::Empty();
    
    // The Javascript source to be linted
    v8::Handle<v8::String> source = v8::String::Empty();

    // Either read the source from file or standard input
    if (NULL != file) {
        // Read from file
        filename = v8::String::New(file);
        source = ReadFile(file);
    } else {
        // Read from stdin
        filename = v8::String::New("Javascript");
        source = ReadInput();
    }
    // The Javascript source is required
    if (source.IsEmpty()) {
        Usage(argv[0]);
        return ERR_NO_SOURCE;
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
    global->Set(v8::String::New("filename"), filename);

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
