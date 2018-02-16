package com.wii64.texpacker.component;

import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Insets;
import java.awt.LayoutManager;
import java.util.HashMap;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 * <p>
 * RiverLayout makes it very simple to construct user interfaces as components
 * are laid out similar to how text is added to a word processor (Components
 * flow like a "river". RiverLayout is however much more powerful than
 * FlowLayout: Components added with the add() method generally gets laid out
 * horizontally, but one may add a string before the com.wii64.texpacker.component being added to
 * specify "constraints" like this: add("br hfill", new
 * JTextField("Your name here"); The code above forces a "line break" and
 * extends the added com.wii64.texpacker.component horizontally. Without the "hfill" constraint, the
 * com.wii64.texpacker.component would take on its preferred size.
 * </p>
 * <p>
 * List of constraints:
 * <ul>
 * <li>br - Add a line break
 * <li>p - Add a paragraph break
 * <li>tab - Add a tab stop (handy for constructing forms with labels followed
 * by fields)
 * <li>hfill - Extend com.wii64.texpacker.component horizontally
 * <li>vfill - Extent com.wii64.texpacker.component vertically (currently only one allowed)
 * <li>left - Align following components to the left (default)
 * <li>center - Align following components horizontally centered
 * <li>right - Align following components to the right
 * <li>vtop - Align following components vertically top aligned
 * <li>vcenter - Align following components vertically centered (default)
 * </ul>
 * </p>
 * RiverLayout is LGPL licenced - use it freely in free and commercial programs
 * 
 * @author David Ekholm
 * @version 1.1 (2005-05-23) -Bugfix: JScrollPanes were oversized (sized to
 *          their containing com.wii64.texpacker.component) if the container containing the
 *          JScrollPane was resized.
 */
@SuppressWarnings("serial")
public class RiverLayout extends FlowLayout implements LayoutManager,
		java.io.Serializable {

	public static final String	LINE_BREAK		= "br";
	public static final String	PARAGRAPH_BREAK	= "p";
	public static final String	TAB_STOP		= "tab";
	public static final String	HFILL			= "hfill";
	public static final String	VFILL			= "vfill";
	public static final String	LEFT			= "left";
	public static final String	RIGHT			= "right";
	public static final String	CENTER			= "center";
	public static final String	VTOP			= "vtop";
	public static final String	VCENTER			= "vcenter";

	Map<Component, String>		constraints		= new HashMap<Component, String>();
	String						valign			= VCENTER;
	int							hgap;
	int							vgap;
	Insets						extraInsets;
	Insets						totalInsets		= new Insets(0, 0, 0, 0);			// Dummy
																					// values.
																					// Set
																					// by
																					// getInsets()

	public RiverLayout() {
		this(10, 5);
	}

	public RiverLayout(int hgap, int vgap) {
		this.hgap = hgap;
		this.vgap = vgap;
		setExtraInsets(new Insets(0, hgap, hgap, hgap));
	}

	/**
	 * Gets the horizontal gap between components.
	 */
	public int getHgap() {
		return hgap;
	}

	/**
	 * Sets the horizontal gap between components.
	 */
	public void setHgap(int hgap) {
		this.hgap = hgap;
	}

	/**
	 * Gets the vertical gap between components.
	 */
	public int getVgap() {
		return vgap;
	}

	public Insets getExtraInsets() {
		return extraInsets;
	}

	public void setExtraInsets(Insets newExtraInsets) {
		extraInsets = newExtraInsets;
	}

	protected Insets getInsets(Container target) {
		Insets insets = target.getInsets();
		totalInsets.top = insets.top + extraInsets.top;
		totalInsets.left = insets.left + extraInsets.left;
		totalInsets.bottom = insets.bottom + extraInsets.bottom;
		totalInsets.right = insets.right + extraInsets.right;
		return totalInsets;
	}

	/**
	 * Sets the vertical gap between components.
	 */
	public void setVgap(int vgap) {
		this.vgap = vgap;
	}

	/**
	 * @param name
	 *            the name of the com.wii64.texpacker.component
	 * @param comp
	 *            the com.wii64.texpacker.component to be added
	 */
	public void addLayoutComponent(String name, Component comp) {
		constraints.put(comp, name);
	}

	/**
	 * Removes the specified com.wii64.texpacker.component from the layout. Not used by this class.
	 * 
	 * @param comp
	 *            the com.wii64.texpacker.component to remove
	 * @see java.awt.Container#removeAll
	 */
	public void removeLayoutComponent(Component comp) {
		constraints.remove(comp);
	}

	boolean isFirstInRow(Component comp) {
		String cons = (String) constraints.get(comp);
		return cons != null
				&& (cons.indexOf(RiverLayout.LINE_BREAK) != -1 || cons
						.indexOf(RiverLayout.PARAGRAPH_BREAK) != -1);
	}

	boolean hasHfill(Component comp) {
		return hasConstraint(comp, RiverLayout.HFILL);
	}

	boolean hasVfill(Component comp) {
		return hasConstraint(comp, RiverLayout.VFILL);
	}

	boolean hasConstraint(Component comp, String test) {
		String cons = (String) constraints.get(comp);
		if (cons == null)
			return false;
		StringTokenizer tokens = new StringTokenizer(cons);
		while (tokens.hasMoreTokens())
			if (tokens.nextToken().equals(test))
				return true;
		return false;
	}

	/**
	 * Figure out tab stop x-positions
	 */
	protected Ruler calcTabs(Container target) {
		Ruler ruler = new Ruler();
		int nmembers = target.getComponentCount();

		int x = 0;
		int tabIndex = 0; // First tab stop
		for (int i = 0; i < nmembers; i++) {
			Component m = target.getComponent(i);
			// if (m.isVisible()) {
			if (isFirstInRow(m) || i == 0) {
				x = 0;
				tabIndex = 0;
			} else
				x += hgap;
			if (hasConstraint(m, TAB_STOP)) {
				ruler.setTab(tabIndex, x); // Will only increase
				x = ruler.getTab(tabIndex++); // Jump forward if neccesary
			}
			Dimension d = m.getPreferredSize();
			x += d.width;
		}
		// }
		return ruler;
	}

	/**
	 * Returns the preferred dimensions for this layout given the <i>visible</i>
	 * components in the specified target container.
	 * 
	 * @param target
	 *            the com.wii64.texpacker.component which needs to be laid out
	 * @return the preferred dimensions to lay out the subcomponents of the
	 *         specified container
	 * @see Container
	 * @see #minimumLayoutSize
	 * @see java.awt.Container#getPreferredSize
	 */
	public Dimension preferredLayoutSize(Container target) {
		synchronized (target.getTreeLock()) {
			Dimension dim = new Dimension(0, 0);
			Dimension rowDim = new Dimension(0, 0);
			int nmembers = target.getComponentCount();
			boolean firstVisibleComponent = true;
			int tabIndex = 0;
			Ruler ruler = calcTabs(target);

			for (int i = 0; i < nmembers; i++) {
				Component m = target.getComponent(i);
				// if (m.isVisible()) {
				if (isFirstInRow(m)) {
					tabIndex = 0;
					dim.width = Math.max(dim.width, rowDim.width);
					dim.height += rowDim.height + vgap;
					if (hasConstraint(m, PARAGRAPH_BREAK))
						dim.height += 2 * vgap;
					rowDim = new Dimension(0, 0);
				}
				if (hasConstraint(m, TAB_STOP))
					rowDim.width = ruler.getTab(tabIndex++);
				Dimension d = m.getPreferredSize();
				rowDim.height = Math.max(rowDim.height, d.height);
				if (firstVisibleComponent) {
					firstVisibleComponent = false;
				} else {
					rowDim.width += hgap;
				}
				rowDim.width += d.width;
				// }
			}
			dim.width = Math.max(dim.width, rowDim.width);
			dim.height += rowDim.height;

			Insets insets = getInsets(target);
			dim.width += insets.left + insets.right;// + hgap * 2;
			dim.height += insets.top + insets.bottom;// + vgap * 2;
			return dim;
		}
	}

	/**
	 * Returns the minimum dimensions needed to layout the <i>visible</i>
	 * components contained in the specified target container.
	 * 
	 * @param target
	 *            the com.wii64.texpacker.component which needs to be laid out
	 * @return the minimum dimensions to lay out the subcomponents of the
	 *         specified container
	 * @see #preferredLayoutSize
	 * @see java.awt.Container
	 * @see java.awt.Container#doLayout
	 */
	public Dimension minimumLayoutSize(Container target) {
		synchronized (target.getTreeLock()) {
			Dimension dim = new Dimension(0, 0);
			Dimension rowDim = new Dimension(0, 0);
			int nmembers = target.getComponentCount();
			boolean firstVisibleComponent = true;
			int tabIndex = 0;
			Ruler ruler = calcTabs(target);

			for (int i = 0; i < nmembers; i++) {
				Component m = target.getComponent(i);
				// if (m.isVisible()) {
				if (isFirstInRow(m)) {
					tabIndex = 0;
					dim.width = Math.max(dim.width, rowDim.width);
					dim.height += rowDim.height + vgap;
					if (hasConstraint(m, PARAGRAPH_BREAK))
						dim.height += 2 * vgap;
					rowDim = new Dimension(0, 0);
				}
				if (hasConstraint(m, TAB_STOP))
					rowDim.width = ruler.getTab(tabIndex++);
				Dimension d = m.getMinimumSize();
				rowDim.height = Math.max(rowDim.height, d.height);
				if (firstVisibleComponent) {
					firstVisibleComponent = false;
				} else {
					rowDim.width += hgap;
				}
				rowDim.width += d.width;
				// }
			}
			dim.width = Math.max(dim.width, rowDim.width);
			dim.height += rowDim.height;

			Insets insets = getInsets(target);
			dim.width += insets.left + insets.right;// + hgap * 2;
			dim.height += insets.top + insets.bottom;// + vgap * 2;
			return dim;
		}
	}

	/**
	 * Centers the elements in the specified row, if there is any slack.
	 * 
	 * @param target
	 *            the com.wii64.texpacker.component which needs to be moved
	 * @param x
	 *            the x coordinate
	 * @param y
	 *            the y coordinate
	 * @param width
	 *            the width dimensions
	 * @param height
	 *            the height dimensions
	 * @param rowStart
	 *            the beginning of the row
	 * @param rowEnd
	 *            the the ending of the row
	 */
	protected void moveComponents(Container target, int x, int y, int width,
			int height, int rowStart, int rowEnd, boolean ltr, Ruler ruler) {
		synchronized (target.getTreeLock()) {
			switch (getAlignment()) {
				case FlowLayout.LEFT:
					x += ltr ? 0 : width;
					break;
				case FlowLayout.CENTER:
					x += width / 2;
					break;
				case FlowLayout.RIGHT:
					x += ltr ? width : 0;
					break;
				case LEADING:
					break;
				case TRAILING:
					x += width;
					break;
			}
			int tabIndex = 0;
			for (int i = rowStart; i < rowEnd; i++) {
				Component m = target.getComponent(i);
				// if (m.isVisible()) {
				if (hasConstraint(m, TAB_STOP))
					x = getInsets(target).left + ruler.getTab(tabIndex++);
				int dy = (valign == VTOP) ? 0 : (height - m.getHeight()) / 2;
				if (ltr) {
					m.setLocation(x, y + dy);
				} else {
					m.setLocation(target.getWidth() - x - m.getWidth(), y + dy);
				}
				x += m.getWidth() + hgap;
				// }
			}
		}
	}

	protected void relMove(Container target, int dx, int dy, int rowStart,
			int rowEnd) {
		synchronized (target.getTreeLock()) {
			for (int i = rowStart; i < rowEnd; i++) {
				Component m = target.getComponent(i);
				// if (m.isVisible()) {
				m.setLocation(m.getX() + dx, m.getY() + dy);
				// }
			}

		}
	}

	protected void adjustAlignment(Component m) {
		if (hasConstraint(m, RiverLayout.LEFT))
			setAlignment(FlowLayout.LEFT);
		else if (hasConstraint(m, RiverLayout.RIGHT))
			setAlignment(FlowLayout.RIGHT);
		else if (hasConstraint(m, RiverLayout.CENTER))
			setAlignment(FlowLayout.CENTER);
		if (hasConstraint(m, RiverLayout.VTOP))
			valign = VTOP;
		else if (hasConstraint(m, RiverLayout.VCENTER))
			valign = VCENTER;

	}

	/**
	 * Lays out the container. This method lets each com.wii64.texpacker.component take its
	 * preferred size by reshaping the components in the target container in
	 * order to satisfy the constraints of this <code>FlowLayout</code> object.
	 * 
	 * @param target
	 *            the specified com.wii64.texpacker.component being laid out
	 * @see Container
	 * @see java.awt.Container#doLayout
	 */
	public void layoutContainer(Container target) {
		setAlignment(FlowLayout.LEFT);
		synchronized (target.getTreeLock()) {
			Insets insets = getInsets(target);
			int maxwidth = target.getWidth() - (insets.left + insets.right);
			int maxheight = target.getHeight() - (insets.top + insets.bottom);

			int nmembers = target.getComponentCount();
			int x = 0, y = insets.top + vgap;
			int rowh = 0, start = 0, moveDownStart = 0;

			boolean ltr = target.getComponentOrientation().isLeftToRight();
			Component toHfill = null;
			Component toVfill = null;
			Ruler ruler = calcTabs(target);
			int tabIndex = 0;

			for (int i = 0; i < nmembers; i++) {
				Component m = target.getComponent(i);
				// if (m.isVisible()) {
				Dimension d = m.getPreferredSize();
				m.setSize(d.width, d.height);

				if (isFirstInRow(m))
					tabIndex = 0;
				if (hasConstraint(m, TAB_STOP))
					x = ruler.getTab(tabIndex++);
				if (!isFirstInRow(m)) {
					if (i > 0 && !hasConstraint(m, TAB_STOP)) {
						x += hgap;
					}
					x += d.width;
					rowh = Math.max(rowh, d.height);
				} else {
					if (toVfill != null && moveDownStart == 0) {
						moveDownStart = i;
					}
					if (toHfill != null) {
						toHfill.setSize(toHfill.getWidth() + maxwidth - x,
								toHfill.getHeight());
						x = maxwidth;
					}
					moveComponents(target, insets.left, y, maxwidth - x, rowh,
							start, i, ltr, ruler);
					x = d.width;
					y += vgap + rowh;
					if (hasConstraint(m, PARAGRAPH_BREAK))
						y += 2 * vgap;
					rowh = d.height;
					start = i;
					toHfill = null;
				}
				// }
				if (hasHfill(m)) {
					toHfill = m;
				}
				if (hasVfill(m)) {
					toVfill = m;
				}
				adjustAlignment(m);
			}

			if (toVfill != null && moveDownStart == 0) { // Don't move anything
															// if hfill
															// com.wii64.texpacker.component is last
															// com.wii64.texpacker.component
				moveDownStart = nmembers;
			}
			if (toHfill != null) { // last com.wii64.texpacker.component
				toHfill.setSize(toHfill.getWidth() + maxwidth - x,
						toHfill.getHeight());
				x = maxwidth;
			}
			moveComponents(target, insets.left, y, maxwidth - x, rowh, start,
					nmembers, ltr, ruler);
			int yslack = maxheight - (y + rowh);
			if (yslack != 0 && toVfill != null) {
				toVfill.setSize(toVfill.getWidth(),
						yslack + toVfill.getHeight());
				relMove(target, 0, yslack, moveDownStart, nmembers);
			}
		}
	}

}

class Ruler {
	private Vector<Integer>	tabs	= new Vector<Integer>();

	public void setTab(int num, int xpos) {
		if (num >= tabs.size())
			tabs.add(num, new Integer(xpos));
		else {
			// Transpose all tabs from this tab stop and onwards
			int delta = xpos - getTab(num);
			if (delta > 0) {
				for (int i = num; i < tabs.size(); i++) {
					tabs.set(i, new Integer(getTab(i) + delta));
				}
			}
		}
	}

	public int getTab(int num) {
		return ((Integer) tabs.get(num)).intValue();
	}

	public String toString() {
		StringBuffer ret = new StringBuffer(getClass().getName() + " {");
		for (int i = 0; i < tabs.size(); i++) {
			ret.append(tabs.get(i));
			if (i < tabs.size() - 1)
				ret.append(',');
		}
		ret.append('}');
		return ret.toString();
	}

	public static void main(String[] args) {
		Ruler r = new Ruler();
		r.setTab(0, 10);
		r.setTab(1, 20);
		r.setTab(2, 30);
		System.out.println(r);
		r.setTab(1, 25);
		System.out.println(r);
		System.out.println(r.getTab(0));
	}
}
