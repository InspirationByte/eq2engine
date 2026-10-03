group "Framework"

-- eqCore essentials
project "coreLib"
	properties { "staticlib", "unitybuild", "concurrency_vis" }
	uses { "public" }
    files {
		"core/**",
	}
	filter "system:Linux"
		links { "pthread" }

-- Framework (Data Structure, Maths, Imaging, Utilities)
project "frameworkLib"
	properties { "staticlib", "unitybuild" }
	uses { "public", "stb" }

    files {
		"ds/**",
        "utils/**",
        "math/**",
        "imaging/**",
		"**.natvis"
	}
	filter "system:Android"
		links { "log" }