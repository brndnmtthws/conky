    /// \name Deprecated Members
    ///@{

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     */
    template <typename S1, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                           const EntryType S1::*member1,
                                                           IT indexes)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                           const EntryType S1::*member1,
                                                           IT indexes, MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1]
                   .gatherArguments(),
               mask);
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param member2 If \p member1 is a struct then \p member2 selects the member to be read from that
     *                struct (i.e. array[i].*member1.*member2 is read).
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     */
    template <typename S1, typename S2, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                           const S2 S1::*member1,
                                                           const EntryType S2::*member2,
                                                           IT indexes)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1][member2]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param member2 If \p member1 is a struct then \p member2 selects the member to be read from that
     *                struct (i.e. array[i].*member1.*member2 is read).
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename S2, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                           const S2 S1::*member1,
                                                           const EntryType S2::*member2,
                                                           IT indexes, MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1][member2]
                   .gatherArguments(),
               mask);
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param ptrMember1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param outerIndexes
     * \param innerIndexes
     */
    template <typename S1, typename IT1, typename IT2>
    Vc_DEPRECATED(
        "use the subscript operator to Vc::array or Vc::vector "
        "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                 const EntryType *const S1::*ptrMember1,
                                                 IT1 outerIndexes, IT2 innerIndexes)
    {
        gather(Common::SubscriptOperation<const S1, IT1, std::ratio<1, 1>, true>(
                   array, outerIndexes)[ptrMember1][innerIndexes]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param ptrMember1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param outerIndexes
     * \param innerIndexes
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename IT1, typename IT2>
    Vc_DEPRECATED(
        "use the subscript operator to Vc::array or Vc::vector "
        "instead.") inline Vc_CURRENT_CLASS_NAME(const S1 *array,
                                                 const EntryType *const S1::*ptrMember1,
                                                 IT1 outerIndexes, IT2 innerIndexes,
                                                 MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT1, std::ratio<1, 1>, true>(
                   array, outerIndexes)[ptrMember1][innerIndexes]
                   .gatherArguments(),
               mask);
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     */
    template <typename S1, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array,
                                                 const EntryType S1::*member1, IT indexes)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array,
                                                 const EntryType S1::*member1,
                                                 IT indexes,
                                                 MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1]
                   .gatherArguments(),
               mask);
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param member2 If \p member1 is a struct then \p member2 selects the member to be read from that
     *                struct (i.e. array[i].*member1.*member2 is read).
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     */
    template <typename S1, typename S2, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array, const S2 S1::*member1,
                                                 const EntryType S2::*member2, IT indexes)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1][member2]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param member1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param member2 If \p member1 is a struct then \p member2 selects the member to be read from that
     *                struct (i.e. array[i].*member1.*member2 is read).
     * \param indexes Determines the offsets into \p array where the values are gathered from/scattered
     *                to. The type of indexes can either be an integer vector or a type that supports
     *                operator[] access.
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename S2, typename IT>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array, const S2 S1::*member1,
                                                 const EntryType S2::*member2, IT indexes,
                                                 MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT, std::ratio<1, 1>, true>(
                   array, indexes)[member1][member2]
                   .gatherArguments(),
               mask);
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param ptrMember1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param outerIndexes
     * \param innerIndexes
     */
    template <typename S1, typename IT1, typename IT2>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array,
                                                 const EntryType *const S1::*ptrMember1,
                                                 IT1 outerIndexes, IT2 innerIndexes)
    {
        gather(Common::SubscriptOperation<const S1, IT1, std::ratio<1, 1>, true>(
                   array, outerIndexes)[ptrMember1][innerIndexes]
                   .gatherArguments());
    }

    /**
     * \deprecated Use Vc::array or Vc::vector subscripting instead.
     *
     * \param array   A pointer into memory (without alignment restrictions).
     * \param ptrMember1 If \p array points to a struct, \p member1 determines the member in the struct to
     *                be read. Thus the offsets in \p indexes are relative to the \p array and not to
     *                the size of the gathered type (i.e. array[i].*member1 is accessed instead of
     *                (&(array->*member1))[i])
     * \param outerIndexes
     * \param innerIndexes
     * \param mask    If a mask is given only the active entries will be gathered/scattered.
     */
    template <typename S1, typename IT1, typename IT2>
    Vc_DEPRECATED("use the subscript operator to Vc::array or Vc::vector "
                  "instead.") inline void gather(const S1 *array,
                                                 const EntryType *const S1::*ptrMember1,
                                                 IT1 outerIndexes, IT2 innerIndexes,
                                                 MaskArgument mask)
    {
        gather(Common::SubscriptOperation<const S1, IT1, std::ratio<1, 1>, true>(
                   array, outerIndexes)[ptrMember1][innerIndexes]
                   .gatherArguments(),
               mask);
    }
    ///@}
