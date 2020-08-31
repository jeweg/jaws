I'm not quite sure how I'm supposed to filter out the structs of privisionary extensions (raytracing).
I can ignore the extension elements, but the enums will still be read. This'll lead to failing
compilation if vulkan_beta.h is not included.