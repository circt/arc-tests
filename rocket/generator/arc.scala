package arc

import freechips.rocketchip.config.Config
import freechips.rocketchip.config.Parameters
import freechips.rocketchip.subsystem._
import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip.util.DontTouch

class BaseConfig extends Config(
  new WithDefaultMemPort ++
  new WithDefaultMMIOPort ++
  new WithNoSlavePort ++
  new WithTimebase(BigInt(1000000)) ++ // 1 MHz
  new WithDTS("freechips,rocketchip-unknown", Nil) ++
  new WithCoherentBusTopology ++
  new BaseSubsystemConfig
)

class SmallConfig extends Config(new WithNSmallCores(1) ++ new BaseConfig)
class MediumConfig extends Config(new WithNMedCores(1) ++ new BaseConfig)
class BigConfig extends Config(new WithNBigCores(1) ++ new BaseConfig)

class RocketSystem(implicit p: Parameters) extends RocketSubsystem
    with CanHaveMasterAXI4MemPort
    with CanHaveMasterAXI4MMIOPort
{
  val bootParams = BootROMParams(
    address = 0x10000,
    size = 0x10000,
    hang = 0x10000,
    contentFileName = "bootrom/bootrom.img"
  )
  val bootROM = BootROM.attach(bootParams, this, CBUS)
  override lazy val module = new RocketSystemImp(this)
}

class RocketSystemImp[+L <: RocketSystem](_outer: L) extends RocketSubsystemModuleImp(_outer)
    with DontTouch
