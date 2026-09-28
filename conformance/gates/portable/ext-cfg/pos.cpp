[[mcpp::cfg(unix)]] int on_unix();                            // expect: ext:cfg
[[mcpp::cfg(windows)]] int on_windows();                      // expect: ext:cfg -- blanked for this target, still a use
[[mcpp::cfg(target_arch = "x86_64")]] int wide();             // expect: ext:cfg
[[mcpp::cfg(any(linux, macos))]] int desktop();               // expect: ext:cfg
[[mcpp::cfg(not(debug_assertions))]] int release();           // expect: ext:cfg
