AM_LDFLAGS	  	= -L$(top_builddir)

INCLUDES 		= -I$(top_srcdir) 


TESTS = \
	Test_all_functions\
	Test_reg_pollfnc\
	Test_reg_pollfnc2\
	Test_reg_pollfnc3\
	Test_reg_pollfnc3_threaded\
        Test_addProperty\
        Test_setProperties\
        Test_setProperties_d\
        Test_regProperties\
	Test_genIndication \
	Test_genIndication_fail


check_PROGRAMS = $(TESTS)



Test_reg_pollfnc_SOURCES = Test_reg_pollfnc.c
Test_reg_pollfnc_LDADD = -lind_helper

Test_reg_pollfnc2_SOURCES = Test_reg_pollfnc2.c 
Test_reg_pollfnc2_LDADD = -lind_helper

Test_reg_pollfnc3_SOURCES = Test_reg_pollfnc3.c
Test_reg_pollfnc3_LDADD = -lind_helper

Test_reg_pollfnc3_threaded_SOURCES = Test_reg_pollfnc3_threaded.c
Test_reg_pollfnc3_threaded_LDADD = -lind_helper

Test_all_functions_SOURCES = Test_all_functions.c
Test_all_functions_LDADD = -lind_helper

Test_addProperty_SOURCES = Test_addProperty.c
Test_addProperty_LDADD = -lind_helper

Test_setProperties_SOURCES = Test_setProperties.c
Test_setProperties_LDADD = -lind_helper

Test_setProperties_d_SOURCES = Test_setProperties_d.c
Test_setProperties_d_LDADD = -lind_helper

Test_regProperties_SOURCES = Test_regProperties.c
Test_regProperties_LDADD = -lind_helper

Test_genIndication_SOURCES = Test_genIndication.c
Test_genIndication_LDADD = -lind_helper

Test_genIndication_fail_SOURCES = Test_genIndication_fail.c
Test_genIndication_fail_LDADD = -lind_helper
