  module DigitalTopWrapper :
    input clock : Clock
    input reset : UInt<1>
    output mem_axi4 : { 0 : { aw : { flip ready : UInt<1>, valid : UInt<1>, bits : { id : UInt<4>, addr : UInt<32>, len : UInt<8>, size : UInt<3>, burst : UInt<2>, lock : UInt<1>, cache : UInt<4>, prot : UInt<3>, qos : UInt<4>, user : { }, echo : { }}}, w : { flip ready : UInt<1>, valid : UInt<1>, bits : { data : UInt<64>, strb : UInt<8>, last : UInt<1>, user : { }}}, flip b : { flip ready : UInt<1>, valid : UInt<1>, bits : { id : UInt<4>, resp : UInt<2>, user : { }, echo : { }}}, ar : { flip ready : UInt<1>, valid : UInt<1>, bits : { id : UInt<4>, addr : UInt<32>, len : UInt<8>, size : UInt<3>, burst : UInt<2>, lock : UInt<1>, cache : UInt<4>, prot : UInt<3>, qos : UInt<4>, user : { }, echo : { }}}, flip r : { flip ready : UInt<1>, valid : UInt<1>, bits : { id : UInt<4>, data : UInt<64>, resp : UInt<2>, user : { }, echo : { }, last : UInt<1>}}}}

    inst top of DigitalTop
    top is invalid

    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_implicit_clock.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_cbus_0.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_mbus_0.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_fbus_0.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_pbus_0.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_sbus_1.clock <= clock
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_sbus_0.clock <= clock

    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_implicit_clock.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_cbus_0.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_mbus_0.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_fbus_0.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_pbus_0.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_sbus_1.reset <= asAsyncReset(reset)
    top.auto.prci_ctrl_domain_tileResetSetter_clock_in.member.allClocks_subsystem_sbus_0.reset <= asAsyncReset(reset)

    top.clock <= clock
    top.reset <= reset
    mem_axi4 <= top.mem_axi4
    top.custom_boot <= UInt<1>("b0")
