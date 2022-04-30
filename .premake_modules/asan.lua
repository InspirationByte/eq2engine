require('vstudio')

-- Add new command line option
newoption {
   trigger     = "Sanitizer",
   value       = "Tool",
   description = "A sanitizer to add to the build",
   allowed     = {
      { "Asan", "Address Sanitizer"}
   }
}

-- Register api command
premake.api.register{
   name="enableASAN",
   scope = "config",
   kind = "boolean",
   default_value = "off",
}

premake.override(premake.vstudio.vc2010, "configurationProperties", function(base, cfg)
   local m = premake.vstudio.vc2010
   m.propertyGroup(cfg, "Configuration")
   premake.callArray(m.elements.configurationProperties, cfg)
   if cfg.enableASAN then
      m.element("EnableASAN", nil, "true")
   end
   premake.pop('</PropertyGroup>')
end)