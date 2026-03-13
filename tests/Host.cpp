

#include "gtest/gtest.h"
#include "socks/LocalHost.h"


TEST(MethodChecking, PopulateObject) {
    LocalHost myMachine {true};
    EXPECT_NE("", myMachine.getName());
    EXPECT_NE("", myMachine.getIPAddress());
    EXPECT_NE("", myMachine.getNetworkMask());
    EXPECT_NE("", myMachine.getMacAddress());
}

TEST(MethodChecking, doNotPopulateObject) {
    LocalHost myMachine {false};
    EXPECT_EQ("", myMachine.getName());
    EXPECT_EQ("", myMachine.getIPAddress());
    EXPECT_EQ("", myMachine.getNetworkMask());
    EXPECT_EQ("", myMachine.getMacAddress());
}

TEST(MethodChecking, selectNonExistentName) {
    LocalHost myMachine {false};
    EXPECT_THROW(try {
        myMachine.getDataFromCurrentHost(2, "This Interface Name will never exit!");
    } catch (GenericException ge) {
        EXPECT_STREQ(ge.what(), "It was not possible to populate the data");
        throw;
    }, GenericException);
}
