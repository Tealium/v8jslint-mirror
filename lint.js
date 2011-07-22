/*global JSLINT:false, print:false, source:false, option:false, filename:false */
(function () {
    "use strict";

    function convert(option) {
        var o,
            toBoolean = function (value) {
                return Boolean(value);
            },
            toNumber  = function (value, defaultValue) {
                var n = Number(value);
                if (isNaN(n)) {
                    return defaultValue;
                }
                return n;
            },
            toArray   = function (value) {
                return value.split(",");
            },
            map = {
                indent    :   { to : toNumber,  defaultValue : 4 },
                maxerr    :   { to : toNumber,  defaultValue : 50 },
                maxlen    :   { to : toNumber,  defaultValue : 80 },
                predef    :   { to : toArray }
            };

        for (o in option) {
            if (option.hasOwnProperty(o)) {
                if (map.hasOwnProperty(o)) {
                    if (map[o].hasOwnProperty("defaultValue")) {
                        // map from String to Number | Array
                        option[o] = map[o].to(option[o], map[o].defaultValue);
                    } else {
                        // map from String to Number | Array
                        option[o] = map[o].to(option[o]);
                    }
                } else {
                    // map from String to Boolean
                    option[o] = toBoolean(option[o]);
                }
            }
        }
    }

    function format(option) {
        var str,
            o;

        str = "";
        for (o in option) {
            if (option.hasOwnProperty(o)) {
                str += o + ": " + option[o].toString() + " ";
            }
        }
        return str;
    }

    function report(data, option) {
        var i,
            e,
            g,
            str;

        if (data.errors) {
            // dump the JSLINT errors
            print("Errors:");
            for (i = 0; i < data.errors.length; i += 1) {
                e = data.errors[i];
                if (e) {
                    str = "Problem at line " + e.line + " character " + e.character +  ": " + e.reason;
                    if (e.evidence) {
                        str += "\n" + e.evidence;
                    }
                    print(str);
                }
            }

            if (data.globals) {
                // dump the JSLINT globals
                print("Globals:");
                for (i = 0; i < data.globals.length; i += 1) {
                    g = data.globals[i];
                    if (g) {
                        print(g);
                    }
                }
            }
        }
    }

    // convert the JSLINT directives from string to boolean | number | array
    convert(option);

    // dump the JSLINT edition
    print("/*jslint " + JSLINT.edition + "*/");

    // run JSLint on given source using given options
    if (!JSLINT(source, option)) {
        report(JSLINT.data(), option);
    } else if (typeof filename !== 'undefined') {
        print(filename + " is good!.");
    } else {
        print("Javascript is good!.");
    }

    // dump the JSLINT directives
    print("/*jslint " + format(option) + "*/");

}());
