package arc

import boom.common._

import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip.devices.debug.HasPeripheryDebug
import freechips.rocketchip.subsystem._
import freechips.rocketchip.util.DontTouch

import org.chipsalliance.cde.config.{Config, Parameters}


class BaseConfig extends Config(
  new chipyard.config.WithPeripheryBusFrequency(500.0) ++    // Default 500 MHz pbus
  new chipyard.config.WithMemoryBusFrequency(500.0) ++       // Default 500 MHz mbus
  new chipyard.config.WithDebugModuleAbstractDataWords(8) ++ // increase debug module data capacity
  new chipyard.config.WithL2TLBs(1024) ++                    // use L2 TLBs
  new chipyard.config.WithInheritBusFrequencyAssignments ++  // Unspecified clocks within a bus will receive the bus frequency if set
  new WithNMemoryChannels(1) ++              // Default 1 memory channels
  new WithNoSlavePort ++                     // no top-level MMIO slave port (overrides default set in rocketchip)
  new WithInclusiveCache ++                  // use Sifive L2 cache
  new WithCoherentBusTopology ++             // hierarchical buses including sbus/mbus/pbus/fbus/cbus/l2
  new WithDTS("ucb-bar,chipyard", Nil) ++    // custom device name for DTS
  new freechips.rocketchip.system.BaseConfig // "base" rocketchip system
)

class SmallConfig extends Config(
  new WithNSmallBooms(1) ++ new BaseConfig)

class MediumConfig extends Config(
  new WithNMediumBooms(1) ++ new BaseConfig)

class LargeConfig extends Config(
  new WithNLargeBooms(1) ++ new BaseConfig)

class MegaConfig extends Config(
  new WithNMegaBooms(1) ++ new BaseConfig)


class BoomSystem(implicit p: Parameters) extends BaseSubsystem
  with HasTiles
  with HasPeripheryDebug
  with CanHaveMasterAXI4MemPort
  with CanHaveMasterAXI4MMIOPort
{
  def coreMonitorBundles = tiles.map {
    case b: BoomTile => b.module.core.coreMonitorBundle
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
  with HasTilesModuleImp
  with DontTouch
