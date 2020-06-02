bwt.o: bwt.cpp bwt.hpp dcsuffixsort.hpp stringsort.hpp partition.hpp \
 difference_cover.hpp dcsamplesort.hpp doubling.hpp distribute.hpp \
 kmpsplit.hpp
ibwt.o: ibwt.cpp ibwt.hpp
dnabwt.o: dnabwt.cpp dnavector.hpp bwt.hpp dcsuffixsort.hpp \
 stringsort.hpp partition.hpp difference_cover.hpp dcsamplesort.hpp \
 doubling.hpp distribute.hpp kmpsplit.hpp
bwtsa.o: bwtsa.cpp suffixsort.hpp doubling.hpp partition.hpp \
 stringsort.hpp checker.hpp difference_cover.hpp timing.hpp
