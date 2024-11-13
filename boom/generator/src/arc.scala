package arc

import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip.devices.debug.HasPeripheryDebug
import freechips.rocketchip.subsystem._
import freechips.rocketchip.util.DontTouch

import org.chipsalliance.cde.config.{Config, Parameters}

import org.chipsalliance.cde.config._
import org.chipsalliance.diplomacy.lazymodule._

class BaseConfig extends Config(
  new chipyard.config.WithDebugModuleAbstractDataWords(8) ++ 
  new chipyard.config.WithL2TLBs(1024) ++
  new freechips.rocketchip.subsystem.WithNMemoryChannels(1) ++ 
  new freechips.rocketchip.subsystem.WithDefaultMemPort ++
  new freechips.rocketchip.subsystem.WithDefaultMMIOPort ++
  new freechips.rocketchip.subsystem.WithNoSlavePort ++
  new freechips.rocketchip.subsystem.WithTimebase(BigInt(1000000)) ++ // 1 MHz
  new freechips.rocketchip.subsystem.WithDTS("ucb-bar,chipyard", Nil) ++
  new freechips.rocketchip.subsystem.WithCoherentBusTopology ++
  new chipyard.config.WithPeripheryBusFrequency(500.0) ++           /** Default 500 MHz pbus */
  new chipyard.config.WithMemoryBusFrequency(500.0) ++              /** Default 500 MHz mbus */
  new chipyard.config.WithControlBusFrequency(500.0) ++             /** Default 500 MHz cbus */
  new chipyard.config.WithSystemBusFrequency(500.0) ++              /** Default 500 MHz sbus */
  new chipyard.config.WithFrontBusFrequency(500.0) ++               /** Default 500 MHz fbus */
  new chipyard.config.WithOffchipBusFrequency(500.0) ++             /** Default 500 MHz obus */
  new chipyard.config.WithInheritBusFrequencyAssignments ++
  new freechips.rocketchip.subsystem.BaseSubsystemConfig
)

//class BaseConfig extends Config(
//  new chipyard.config.WithDebugModuleAbstractDataWords(8) ++                // increase debug module data capacity
//  new chipyard.config.WithL2TLBs(1024) ++                                   // use L2 TLBs
//                // Unspecified clocks within a bus will receive the bus frequency if set
//  new freechips.rocketchip.subsystem.WithNMemoryChannels(1) ++              // Default 1 memory channels
//  new freechips.rocketchip.subsystem.WithNoSlavePort ++
// // new freechips.rocketchip.subsystem.WithInclusiveCache ++                  // use Sifive L2 cache
//  new freechips.rocketchip.subsystem.WithCoherentBusTopology ++    
//  new freechips.rocketchip.subsystem.WithDTS("ucb-bar,chipyard", Nil) ++    // custom device name for DTSA
//   new chipyard.clocking.WithClockGroupsCombinedByName(("uncore",        /** create a "uncore" clock group tieing all the bus clocks together */
//   Seq("sbus", "mbus", "pbus", "fbus", "cbus", "obus", "implicit", "clock_tap"), Seq("tile"))) ++
//  // new WithDontDriveBusClocksFromSBus ++
//  // new chipyard.clocking.WithPassthroughClockGenerator ++
//  // new chipyard.config.WithNoSubsystemClockIO ++
//    new chipyard.config.WithPeripheryBusFrequency(500.0) ++           /** Default 500 MHz pbus */
//  new chipyard.config.WithMemoryBusFrequency(500.0) ++              /** Default 500 MHz mbus */
//  new chipyard.config.WithControlBusFrequency(500.0) ++             /** Default 500 MHz cbus */
//  new chipyard.config.WithSystemBusFrequency(500.0) ++              /** Default 500 MHz sbus */
//  new chipyard.config.WithFrontBusFrequency(500.0) ++               /** Default 500 MHz fbus */
//  new chipyard.config.WithOffchipBusFrequency(500.0) ++             /** Default 500 MHz obus */
//  new chipyard.config.WithInheritBusFrequencyAssignments ++
//  new chipyard.config.WithTileFrequency(500.0) ++
//  new freechips.rocketchip.system.BaseConfig // "base" rocketchip system
//)

class SmallConfig extends Config(
  new boom.v3.common.WithNSmallBooms(1) ++ new BaseConfig)

class MediumConfig extends Config(
  new boom.v3.common.WithNMediumBooms(1) ++ new BaseConfig)

class LargeConfig extends Config(
  new boom.v3.common.WithNLargeBooms(1) ++ new BaseConfig)

class MegaConfig extends Config(
  new boom.v3.common.WithNMegaBooms(1) ++ new BaseConfig)

  
class DualSmallConfig extends Config(
  new boom.v3.common.WithNSmallBooms(2) ++ new BaseConfig)

class DualMediumConfig extends Config(
  new boom.v3.common.WithNMediumBooms(2) ++ new BaseConfig)

class DualLargeConfig extends Config(
  new boom.v3.common.WithNLargeBooms(2) ++ new BaseConfig)

class DualMegaConfig extends Config(
  new boom.v3.common.WithNMegaBooms(2) ++ new BaseConfig)


class BoomSystem(implicit p: Parameters) extends BaseSubsystem
  with InstantiatesHierarchicalElements
  with HasTileNotificationSinks
  with HasTileInputConstants
  with CanHavePeripheryCLINT
  with CanHavePeripheryPLIC
  with HasPeripheryDebug
  with HasHierarchicalElementsRootContext
  with HasHierarchicalElements
  with freechips.rocketchip.util.HasCoreMonitorBundles
  with CanHaveMasterAXI4MemPort
  with CanHaveMasterAXI4MMIOPort
{
  def coreMonitorBundles = totalTiles.values.map {
    case b: boom.v3.common.BoomTile => b.module.core.coreMonitorBundle
  }.toList

  val bootParams = BootROMParams(
    address = 0x10000,
    size = 0x10000,
    hang = 0x10000,
    contentFileName = "bootrom.rv64.img"
  )
  val bootROM  = BootROM.attach(bootParams, this, CBUS)

  override lazy val module = new BoomSystemModule(this)
}

class BoomSystemModule[+L <: BoomSystem](_outer: L) extends BaseSubsystemModuleImp(_outer)
  with DontTouch
  with HasHierarchicalElementsRootContextModuleImp {
    override lazy val outer = _outer
  }
