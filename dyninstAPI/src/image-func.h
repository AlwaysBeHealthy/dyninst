/*
 * Copyright (c) 1996-2004 Barton P. Miller
 * 
 * We provide the Paradyn Parallel Performance Tools (below
 * described as "Paradyn") on an AS IS basis, and do not warrant its
 * validity or performance.  We reserve the right to update, modify,
 * or discontinue this software at any time.  We shall have no
 * obligation to supply such updates or modifications or any other
 * form of support to you.
 * 
 * This license is for research uses.  For such uses, there is no
 * charge. We define "research use" to mean you may freely use it
 * inside your organization for whatever purposes you see fit. But you
 * may not re-distribute Paradyn or parts of Paradyn, in any form
 * source or binary (including derivatives), electronic or otherwise,
 * to any other organization or entity without our permission.
 * 
 * (for other uses, please contact us at paradyn@cs.wisc.edu)
 * 
 * All warranties, including without limitation, any warranty of
 * merchantability or fitness for a particular purpose, are hereby
 * excluded.
 * 
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 * 
 * Even if advised of the possibility of such damages, under no
 * circumstances shall we (or any other person or entity with
 * proprietary rights in the software licensed hereunder) be liable
 * to you or any third party for direct, indirect, or consequential
 * damages of any character regardless of type of action, including,
 * without limitation, loss of profits, loss of use, loss of good
 * will, or computer failure or malfunction.  You agree to indemnify
 * us (and any other person or entity with proprietary rights in the
 * software licensed hereunder) for any and all liability it may
 * incur to third parties resulting from your use of Paradyn.
 */
 
// $Id: image-func.h,v 1.7 2005/12/06 20:01:19 bernat Exp $

#ifndef IMAGE_FUNC_H
#define IMAGE_FUNC_H

#include "common/h/String.h"
#include "common/h/Vector.h"
#include "common/h/Types.h"
#include "common/h/Pair.h"
#include "codeRange.h"
#include "arch.h" // instruction

#if !defined(BPATCH_LIBRARY)
#include "paradynd/src/resource.h"
#endif

#include "dyninstAPI/h/BPatch_Set.h"
#include "common/h/Dictionary.h"

class pdmodule;

// Slight modifications (and specialization) of BPatch_*
class image_basicBlock;
class image_instPoint;

class image_basicBlock : public codeRange {
    friend class image_func;
 public:
    image_basicBlock(image_func *func, Address firstOffset);

    Address firstInsnOffset() const { return firstInsnOffset_; }
    Address lastInsnOffset() const { return lastInsnOffset_; }
    // Just to be obvious -- this is the end addr of the block
    Address endOffset() const { return blockEndOffset_; }
    Address getSize() const { return blockEndOffset_ - firstInsnOffset_; }
    
    bool isEntryBlock() const { return isEntryBlock_; }
    bool isExitBlock() const { return isExitBlock_; }

    static int compare(image_basicBlock *&b1,
                       image_basicBlock *&b2) {
        if (b1->firstInsnOffset() < b2->firstInsnOffset())
            return -1;
        if (b2->firstInsnOffset() < b1->firstInsnOffset())
            return 1;
        assert(b1 == b2);
        return 0;
    }

    void debugPrint();

    void getSources(pdvector<image_basicBlock *> &ins) const;
    void getTargets(pdvector<image_basicBlock *> &outs) const;

    Address get_address_cr() const { return firstInsnOffset(); }
    unsigned get_size_cr() const { return getSize(); }
    void *getPtrToInstruction(Address addr) const;

    void addTarget(image_basicBlock *target);
    void addSource(image_basicBlock *source);

    void removeTarget(image_basicBlock * target);
    void removeSource(image_basicBlock * source);

    int id() const { return blockNumber_; }

    image_func *func() const { return func_; }

    // Try to shrink memory usage down.
    void finalize();

 private:
    Address firstInsnOffset_;
    Address lastInsnOffset_;
    Address blockEndOffset_;

    bool isEntryBlock_;
    bool isExitBlock_;

    int blockNumber_;

    pdvector<image_basicBlock *> targets_;
    pdvector<image_basicBlock *> sources_;

    image_func *func_;
};

void checkIfRelocatable (instruction insn, bool &canBeRelocated);
bool isRealCall(instruction insn, Address addr, image *owner, bool &validTarget);

// Parse-level function object. Knows about offsets, names, and suchlike; 
// does _not_ do function relocation.

class image_func : public codeRange {
 public:
   static pdstring emptyString;

   // Almost everything gets filled in later.
   image_func(const pdstring &symbol, 
	      Address offset, 
	      const unsigned symTabSize, 
	      pdmodule *m,
	      image *i);
   

   ~image_func();

   ////////////////////////////////////////////////
   // Basic output functions
   ////////////////////////////////////////////////

   const pdstring &symTabName() const { 
       if (symTabNames_.size() > 0) return symTabNames_[0];
       else return emptyString;
   }
   const pdstring &prettyName() const {
       if (prettyNames_.size() > 0) return prettyNames_[0];
       else return emptyString;
   }
   const pdstring &typedName() const {
       if (typedNames_.size() > 0) return typedNames_[0];
       else return emptyString;
   }

   const pdvector<pdstring> &symTabNameVector() const { return symTabNames_; }
   const pdvector<pdstring> &prettyNameVector() const { return prettyNames_; }
   const pdvector<pdstring> &typedNameVector() const { return typedNames_; }
   void copyNames(image_func *duplicate);

   // Bool: returns true if the name is new (and therefore added)
   bool addSymTabName(pdstring name, bool isPrimary = false);
   bool addPrettyName(pdstring name, bool isPrimary = false);
   bool addTypedName(pdstring name, bool isPrimary = false);

   Address getOffset() const {return startOffset_;}
   Address getEndOffset(); // May trigger parsing
   unsigned getSymTabSize() const { return symTabSize_; }

   // coderange needs a get_address...
   Address get_address_cr() const { return getOffset();}
   unsigned get_size_cr() const { return symTabSize_; } // May be
                                                        // incorrect
                                                        // but is
                                                        // consistent.
   void *getPtrToInstruction(Address addr) const;


   // extra debuggering info....
   ostream & operator<<(ostream &s) const;
   friend ostream &operator<<(ostream &os, image_func &f);
   pdmodule *pdmod() const { return mod_;}
   image *img() const { return image_; }
   void changeModule(pdmodule *mod);

   ////////////////////////////////////////////////
   // CFG and other function body methods
   ////////////////////////////////////////////////

   const pdvector< image_basicBlock* > &blocks() const{ return blockList; }

   bool hasNoStackFrame() const {return noStackFrame;}
   bool makesNoCalls() const {return makesNoCalls_;}
   bool savesFramePointer() const {return savesFP_;}

   ////////////////////////////////////////////////
   // Instpoints!
   ////////////////////////////////////////////////

   const pdvector<image_instPoint*> &funcEntries();
   const pdvector<image_instPoint*> &funcExits();
   const pdvector<image_instPoint*> &funcCalls();
   
   // Defined in inst-<arch>.C
   // The address vector is "possible call targets you might be interested in".
   bool findInstPoints( pdvector<Address >& );

   // Helper function: create a new basic block and add to various data structures
   // (if the new addr is valid)
   bool addBasicBlock(Address newAddr,
                      image_basicBlock *oldBlock,
                      BPatch_Set<Address> &leaders,
                      dictionary_hash<Address, image_basicBlock *> &leadersToBlock,
                      pdvector<Address> &jmpTargets); 
   // And it uses blockList as well

   // Platform-independent sorting/cleaning/fixing of instPoints and
   // basic blocks.
   bool cleanBlockList();
   void checkCallPoints();
   Address newCallPoint(Address adr, const instruction code, bool &err);
   
   void canFuncBeInstrumented( bool b ) { isInstrumentable_ = b; };

   bool isTrapFunc() const {return isTrap;}
   bool isInstrumentable() const { return isInstrumentable_; }

#if defined(cap_stripped_binaries)
   // Update if symtab is incorrect
   void updateFunctionEnd( Address addr);  
#endif

   // ----------------------------------------------------------------------

   ////////////////////////////////////////////////
   // Misc
   ////////////////////////////////////////////////

   codeRange *copy() const;

#if defined(arch_ia64) || defined(arch_x86) || defined(arch_x86_64)

   bool isTrueCallInsn(const instruction insn);
#endif

#if defined(sparc_sun_solaris2_4)
   bool is_o7_live(){ return o7_live; }
#endif

   // Only defined on alpha right now
   int             frame_size; // stack frame size

#if defined(arch_ia64)
   // We need to know where all the alloc instructions in the
   // function are to do a reasonable job of register allocation
   // in the base tramp.  
   pdvector< Address > allocs;
   
   // Since the IA-64 ABI does not define a frame pointer register,
   // we use DWARF debug records (DW_AT_frame_base entries) to 
   // construct an AST which calculates the frame pointer.
   AstNode * framePointerCalculator;
   
   // Place to store the results of doFloatingPointStaticAnalysis().
   // This way, if they are ever needed in a mini-tramp, emitFuncJump()
   // for example, the expensive operation doesn't need to happen again.
   bool * usedFPregs;
#endif

   // Ifdef relocation... but set at parse time.
   bool canBeRelocated() const { return canBeRelocated_; }
   bool needsRelocation() const { return needsRelocation_; }
   void markAsNeedingRelocation(bool foo) { needsRelocation_ = foo; }


 private:

   ///////////////////// Basic func info
   pdvector<pdstring> symTabNames_;	/* name as it appears in the symbol table */
   pdvector<pdstring> prettyNames_;	/* user's view of name (i.e. de-mangled) */
   pdvector<pdstring> typedNames_;      /* de-mangled with types */
   Address startOffset_;		/* address of the start of the func */
   Address endOffset_;          /* Address of the (next instruction after) the end of the func */
   unsigned symTabSize_;        /* What we get from the symbol table (if any) */
   pdmodule *mod_;		/* pointer to file that defines func. */
   image *image_;
   bool parsed_;                /* Set to true in findInstPoints */


   ///////////////////// CFG and function body
   pdvector< image_basicBlock* > blockList;

   bool noStackFrame; // formerly "leaf".  True iff this fn has no stack frame.
   bool makesNoCalls_;
   bool savesFP_;
   bool call_points_have_been_checked; // true if checkCallPoints has been called.

   
   ///////////////////// Instpoints 

   pdvector<image_instPoint*> funcEntries_;     /* place to instrument entry 
                                                   (often not addr) */
   pdvector<image_instPoint*> funcReturns;	/* return point(s). */
   pdvector<image_instPoint*> calls;		/* pointer to the calls */
   
   bool isTrap; 		// true if function contains a trap instruct
   bool isInstrumentable_;     // true if the function is instrumentable
   
   bool canBeRelocated_;           // True if nothing prevents us from
                                   // relocating function
   bool needsRelocation_;          // Override -- "relocate this func"

   void *originalCode;   // points to copy of original function

   // Hacky way around parsing things -- we can stick things known to be 
   // unparseable in here.
   bool isInstrumentableByFunctionName();

   // Misc   

   // the vector "calls" should not be accessed until this is true.
   bool o7_live;

};

typedef image_func *ifuncPtr;

struct ifuncCmp
{
    int operator()( const ifuncPtr &f1, const ifuncPtr &f2 ) const
    {
        if( f1->getOffset() > f2->getOffset() )
            return 1;
        if( f1->getOffset() < f2->getOffset() )
            return -1;
        return 0;
    }
};

#endif /* FUNCTION_H */
