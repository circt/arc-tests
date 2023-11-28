package arc

import boom.common._

import freechips.rocketchip.devices.tilelink._
import freechips.rocketchip.devices.debug.HasPeripheryDebug
import freechips.rocketchip.subsystem._
import freechips.rocketchip.util.DontTouch

import org.chipsalliance.cde.config.{Config, Parameters}

import org.chipsalliance.cde.config.{Parameters, Config, Field}
import freechips.rocketchip.subsystem._
import freechips.rocketchip.devices.tilelink.{BootROMParams}
import freechips.rocketchip.diplomacy.{SynchronousCrossing, AsynchronousCrossing, RationalCrossing}
import freechips.rocketchip.rocket._
import freechips.rocketchip.tile._

import boom.ifu._
import boom.exu._
import boom.lsu._


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

class WithNGigaBooms(n: Int = 1, overrideIdOffset: Option[Int] = None) extends Config(
  new WithTAGELBPD ++ // Default to TAGE-L BPD
  new Config((site, here, up) => {
    case TilesLocated(InSubsystem) => {
      val prev = up(TilesLocated(InSubsystem), site)
      val idOffset = overrideIdOffset.getOrElse(prev.size)
      (0 until n).map { i =>
        BoomTileAttachParams(
          tileParams = BoomTileParams(
            core = BoomCoreParams(
              fetchWidth = 8,
              decodeWidth = 5,
              numRobEntries = 130,
              issueParams = Seq(
                IssueParams(issueWidth=2, numEntries=24, iqType=IQT_MEM.litValue, dispatchWidth=5),
                IssueParams(issueWidth=5, numEntries=40, iqType=IQT_INT.litValue, dispatchWidth=5),
                IssueParams(issueWidth=2, numEntries=32, iqType=IQT_FP.litValue , dispatchWidth=5)),
              numIntPhysRegisters = 128,
              numFpPhysRegisters = 128,
              numLdqEntries = 32,
              numStqEntries = 32,
              maxBrCount = 20,
              numFetchBufferEntries = 35,
              enablePrefetching = true,
              // numDCacheBanks = 1,
              ftq = FtqParameters(nEntries=40),
              fpu = Some(freechips.rocketchip.tile.FPUParams(sfmaLatency=4, dfmaLatency=4, divSqrt=true))
            ),
            dcache = Some(
              DCacheParams(rowBits = 128, nSets=64, nWays=8, nMSHRs=8, nTLBWays=32)
            ),
            icache = Some(
              ICacheParams(rowBits = 128, nSets=64, nWays=8, fetchBytes=4*4)
            ),
            hartId = i + idOffset
          ),
          crossingParams = RocketCrossingParams()
        )
      } ++ prev
    }
    case XLen => 64
  })
)

class SmallConfig extends Config(
  new WithNSmallBooms(1) ++ new BaseConfig)

class MediumConfig extends Config(
  new WithNMediumBooms(1) ++ new BaseConfig)

class LargeConfig extends Config(
  new WithNLargeBooms(1) ++ new BaseConfig)

class MegaConfig extends Config(
  new WithNMegaBooms(1) ++ new BaseConfig)

class GigaConfig extends Config(
  new arc.WithNGigaBooms(1) ++ new BaseConfig)


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
