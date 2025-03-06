group "Framework"

-- eqCore essentials
project "coreLib"
    kind "StaticLib"
	properties { "unitybuild", "concurrency_vis" }
	uses { "public" }
    files {
		"core/**",
	}
	filter "system:Linux"
		links { "pthread" }

-- Framework (Data Structure, Maths, Imaging, Utilities)
project "frameworkLib"
    kind "StaticLib"
	properties { "unitybuild" }
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